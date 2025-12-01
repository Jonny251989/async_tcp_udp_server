#include "server.hpp"

std::atomic<bool> global_shutdown_flag{false};

void signal_handler(int signal) {
    static std::atomic<bool> already_called{false};
    if (already_called.exchange(true)) {
        return;
    }
    
    std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
    global_shutdown_flag.store(true, std::memory_order_release);
}   

Server::Server(uint16_t port) 
    : port_(port)
    , session_manager_(std::make_shared<SessionManager>())
    , command_processor_(create_commands())
    , shutdown_requested_(false) {}

Server::~Server() {
    stop();
}

std::vector<std::unique_ptr<Command>> Server::create_commands() {
    std::vector<std::unique_ptr<Command>> commands;
    commands.push_back(std::make_unique<TimeCommand>());
    commands.push_back(std::make_unique<StatsCommand>(*session_manager_));
    commands.push_back(std::make_unique<ShutdownCommand>());
    return commands;
}

bool Server::start() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    std::signal(SIGQUIT, signal_handler);

    std::signal(SIGPIPE, SIG_IGN);
    
    tcp_handler_ = std::make_unique<TcpHandler>(port_, session_manager_);
    udp_handler_ = std::make_unique<UdpHandler>(port_);
    
    if (!tcp_handler_->start() || !udp_handler_->start()) {
        std::cerr << "Failed to start TCP or UDP handler" << std::endl;
        return false;
    }
    
    setup_tcp_handler();
    setup_udp_handler();
    
    return true;
}

void Server::stop() {
    if (shutdown_requested_) return;
    
    shutdown_requested_ = true;
    
    // Останавливаем обработчики
    if (tcp_handler_) tcp_handler_->stop();
    if (udp_handler_) udp_handler_->stop();
}

void Server::run() {
    // Главный цикл с ЧАСТОЙ проверкой флагов
    while (true) {
        // Проверяем флаги ПЕРЕД каждым вызовом event_loop
        if (shutdown_requested_ || global_shutdown_flag.load(std::memory_order_acquire)) {
            break;
        }
        
        // Обрабатываем события с очень коротким таймаутом
        event_loop_.run(1);  // 1ms timeout
        
        // Проверяем флаги ПОСЛЕ каждого вызова event_loop
        if (shutdown_requested_ || global_shutdown_flag.load(std::memory_order_acquire)) {
            break;
        }
    }
    
    // Остановка при выходе из цикла
    stop();
}

void Server::setup_tcp_handler() {
    tcp_handler_->set_connection_callback([this](auto connection) {
        handle_tcp_connection(connection);
    });
    
    event_loop_.add_fd(tcp_handler_->get_socket_fd(), EPOLLIN, [this](uint32_t events) {
        if (events & EPOLLIN) tcp_handler_->handle_accept();
    });
}

void Server::setup_udp_handler() {
    udp_handler_->set_message_callback([this](const auto& message, const auto& client_addr) {
        handle_udp_message(message, client_addr);
    });
    
    event_loop_.add_fd(udp_handler_->get_socket_fd(), EPOLLIN, [this](uint32_t events) {
        if (events & EPOLLIN) udp_handler_->handle_message();
    });
}

void Server::handle_tcp_connection(std::shared_ptr<TcpConnection> connection) {
    connection->set_message_callback([this, connection](const auto& message) {
        handle_tcp_message(message, connection);
    });
    
    connection->set_close_callback([this, connection]() {
        event_loop_.remove_fd(connection->get_fd());
    });
    
    event_loop_.add_fd(connection->get_fd(), EPOLLIN, [connection](uint32_t events) {
        if (events & EPOLLIN) connection->handle_read();
    });
}

void Server::handle_tcp_message(const std::string& message, std::shared_ptr<TcpConnection> connection) {
    std::string response = command_processor_.process_command(message);
    
    if (response == "/SHUTDOWN_ACK") {
        shutdown_requested_ = true;
        connection->send("Server shutting down gracefully...\n");
        global_shutdown_flag.store(true, std::memory_order_release);
        return;
    }
    
    connection->send(response + "\n");
}

void Server::handle_udp_message(const std::string& message, const sockaddr_in& client_addr) {
    std::string response = command_processor_.process_command(message);
    
    if (response == "/SHUTDOWN_ACK") {
        shutdown_requested_ = true;
        udp_handler_->send_message("Server shutting down gracefully...", client_addr);
        global_shutdown_flag.store(true, std::memory_order_release);
        return;
    }
    
    udp_handler_->send_message(response, client_addr);
}