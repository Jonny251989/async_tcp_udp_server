#include "server.hpp"
#include <csignal>
#include <iostream>

std::atomic<bool> global_shutdown_flag{false};

void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down gracefully..." << std::endl;
    global_shutdown_flag.store(true, std::memory_order_release);
}

Server::Server(uint16_t port) 
    : port_(port)
    , session_manager_(std::make_shared<SessionManager>())
    , command_processor_(create_commands())
    , shutdown_requested_(false) {
    
    std::cout << "=== SERVER CONSTRUCTOR ===" << std::endl;
    std::cout << "Creating commands..." << std::endl;
    
    auto commands = create_commands();
    for (const auto& cmd : commands) {
        std::cout << "Command: " << cmd->name() << std::endl;
    }
}

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
    // Обработка сигналов
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    std::signal(SIGQUIT, signal_handler);
    
    // Игнорируем SIGPIPE
    std::signal(SIGPIPE, SIG_IGN);
    
    tcp_handler_ = std::make_unique<TcpHandler>(port_, session_manager_);
    udp_handler_ = std::make_unique<UdpHandler>(port_);
    
    if (!tcp_handler_->start() || !udp_handler_->start()) {
        std::cerr << "Failed to start TCP or UDP handler" << std::endl;
        return false;
    }
    
    setup_tcp_handler();
    setup_udp_handler();
    
    std::cout << "Server started on port " << port_ << std::endl;
    return true;
}

void Server::stop() {
    if (shutdown_requested_) {
        return;
    }
    
    shutdown_requested_ = true;
    std::cout << "SERVER: Stop() called - initiating shutdown sequence" << std::endl;
    
    // Останавливаем event loop первым
    std::cout << "SERVER: Stopping event loop..." << std::endl;
    event_loop_.stop();
    
    // Быстро останавливаем обработчики
    std::cout << "SERVER: Stopping TCP handler..." << std::endl;
    if (tcp_handler_) {
        tcp_handler_->stop();
    }
    
    std::cout << "SERVER: Stopping UDP handler..." << std::endl;
    if (udp_handler_) {
        udp_handler_->stop();
    }
    
    std::cout << "SERVER: All components stopped successfully" << std::endl;
}

void Server::run() {
    std::cout << "Server entering main event loop..." << std::endl;
    
    auto last_activity_check = std::chrono::steady_clock::now();
    int iteration = 0;
    
    while (!shutdown_requested_ && !global_shutdown_flag.load(std::memory_order_acquire)) {
        iteration++;
        
        // Обрабатываем события с коротким таймаутом
        event_loop_.run(50);  // Уменьшили таймаут до 50ms
        
        // Проверяем флаги чаще
        if (shutdown_requested_ || global_shutdown_flag.load(std::memory_order_acquire)) {
            std::cout << "Shutdown detected in main loop, breaking immediately" << std::endl;
            break;
        }
        
        // Небольшая пауза, но короче
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        
        // Логируем раз в 5 секунд
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_activity_check).count() >= 5) {
            std::cout << "Server active - iterations: " << iteration 
                      << ", shutdown_requested_: " << shutdown_requested_
                      << ", global_shutdown_flag: " << global_shutdown_flag.load() 
                      << std::endl;
            last_activity_check = now;
        }
    }
    
    std::cout << "SERVER: Exiting main loop. Starting cleanup..." << std::endl;
    stop();
    std::cout << "SERVER: Cleanup completed. Process exiting." << std::endl;
    
    // Явно завершаем процесс после остановки
    std::exit(0);
}


void Server::setup_tcp_handler() {
    std::cout << "Server::setup_tcp_handler() - Setting up TCP handler callbacks" << std::endl;
    
    tcp_handler_->set_connection_callback([this](auto connection) {
        std::cout << "TCP Connection callback triggered" << std::endl;
        handle_tcp_connection(connection);
    });
    
    int tcp_fd = tcp_handler_->get_socket_fd();
    std::cout << "Adding TCP socket fd " << tcp_fd << " to event loop" << std::endl;
    
    bool added = event_loop_.add_fd(tcp_fd, EPOLLIN, [this](uint32_t events) {
        std::cout << "TCP Event callback - events: " << events << std::endl;
        if (events & EPOLLIN) {
            std::cout << "TCP EPOLLIN event - calling handle_accept()" << std::endl;
            tcp_handler_->handle_accept();
        }
    });
    
    if (added) {
        std::cout << "✓ TCP socket successfully added to event loop" << std::endl;
    } else {
        std::cerr << "❌ Failed to add TCP socket to event loop" << std::endl;
    }
}

void Server::setup_udp_handler() {
    std::cout << "Server::setup_udp_handler() - Setting up UDP handler callbacks" << std::endl;
    
    udp_handler_->set_message_callback([this](const auto& message, const auto& client_addr) {
        std::cout << "UDP Message callback triggered" << std::endl;
        handle_udp_message(message, client_addr);
    });
    
    int udp_fd = udp_handler_->get_socket_fd();
    std::cout << "Adding UDP socket fd " << udp_fd << " to event loop" << std::endl;
    
    bool added = event_loop_.add_fd(udp_fd, EPOLLIN, [this](uint32_t events) {
        std::cout << "UDP Event callback - events: " << events << std::endl;
        if (events & EPOLLIN) {
            std::cout << "UDP EPOLLIN event - calling handle_message()" << std::endl;
            udp_handler_->handle_message();
        }
    });
    
    if (added) {
        std::cout << "✓ UDP socket successfully added to event loop" << std::endl;
    } else {
        std::cerr << "❌ Failed to add UDP socket to event loop" << std::endl;
    }
}

void Server::handle_tcp_connection(std::shared_ptr<TcpConnection> connection) {
    std::cout << "Server::handle_tcp_connection() - Setting up connection callbacks" << std::endl;
    
    connection->set_message_callback([this, connection](const auto& message) {
        std::cout << "Server: message_callback triggered for connection" << std::endl;
        handle_tcp_message(message, connection);
    });
    
    connection->set_close_callback([this, connection]() {
        std::cout << "Server: close_callback triggered" << std::endl;
        event_loop_.remove_fd(connection->get_fd());
    });
    
    std::cout << "Server: Adding fd " << connection->get_fd() << " to event loop" << std::endl;
    event_loop_.add_fd(connection->get_fd(), EPOLLIN, [connection](uint32_t events) {
        std::cout << "Server: Event callback for fd " << connection->get_fd() << ", events: " << events << std::endl;
        if (events & EPOLLIN) {
            std::cout << "Server: EPOLLIN event, calling handle_read()" << std::endl;
            connection->handle_read();
        }
    });
    
    std::cout << "Server::handle_tcp_connection() - COMPLETE" << std::endl;
}

void Server::handle_tcp_message(const std::string& message, std::shared_ptr<TcpConnection> connection) {
    std::string response = command_processor_.process_command(message);
    
    if (response == "/SHUTDOWN_ACK") {
        std::cout << "SHUTDOWN COMMAND RECEIVED - initiating immediate shutdown" << std::endl;
        shutdown_requested_ = true;
        connection->send("Server shutting down gracefully...\n");
        
        // Немедленный выход из процесса
        std::cout << "Exiting process due to shutdown command" << std::endl;
        std::exit(0);
    }
    
    connection->send(response + "\n");
}

void Server::handle_udp_message(const std::string& message, const sockaddr_in& client_addr) {
    std::cout << "Handling UDP message: '" << message << "'" << std::endl;
    
    std::string response = command_processor_.process_command(message);
    
    if (response == "/SHUTDOWN_ACK") {
        std::cout << "!!! SHUTDOWN COMMAND RECEIVED via UDP !!!" << std::endl;
        shutdown_requested_ = true;
        udp_handler_->send_message("Server shutting down gracefully...", client_addr);
        return;
    }
    
    udp_handler_->send_message(response, client_addr);
}