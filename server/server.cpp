#include "server.hpp"

#include <iostream>
#include <cstdlib>
#include <memory>
#include <csignal>

Server::Server(uint16_t port) 
    : port_(port)
    , session_manager_(std::make_shared<SessionManager>())
    , command_processor_(create_commands())
    , shutdown_requested_(false) {
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
    // Ignore SIGPIPE (to avoid crashing on broken connections)
    std::signal(SIGPIPE, SIG_IGN);
    
    // Setup signal handler first
    setup_signal_handler();
    
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
    
    if (tcp_handler_) tcp_handler_->stop();
    if (udp_handler_) udp_handler_->stop();
    
    // Reset signal handler when stopping
    reset_signal_handler();
}

void Server::run() {
    while (true) {
        if (shutdown_requested_) {
            break;
        }
        
        try {
            event_loop_.run(100); // 100ms timeout to check flags
        } catch (const std::exception& e) {
            std::cerr << "Event loop error: " << e.what() << std::endl;
            break;
        }
        
        if (shutdown_requested_) {
            break;
        }
    }
    
    stop();
}

void Server::request_shutdown() {
    std::cout << "\nShutdown requested via signal..." << std::endl;
    shutdown_requested_ = true;
}

void Server::setup_signal_handler() {
    // Use weak_ptr for safe access from signal handler
    std::weak_ptr<Server> weak_this = shared_from_this();
    
    set_signal_handler({SIGINT, SIGTERM, SIGQUIT}, [weak_this]() {
        if (auto server = weak_this.lock()) {
            server->request_shutdown();
        }
    });
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
        return;
    }
    
    connection->send(response + "\n");
}

void Server::handle_udp_message(const std::string& message, const sockaddr_in& client_addr) {
    std::string response = command_processor_.process_command(message);
    
    if (response == "/SHUTDOWN_ACK") {
        shutdown_requested_ = true;
        udp_handler_->send_message("Server shutting down gracefully...", client_addr);
        return;
    }
    
    udp_handler_->send_message(response, client_addr);
}