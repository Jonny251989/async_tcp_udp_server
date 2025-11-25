#include "server.hpp"
#include <iostream>
#include <atomic>

std::atomic<bool> global_shutdown_flag{false};

void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    global_shutdown_flag = true;
}

Server::Server(uint16_t port) 
    : port_(port)
    , command_processor_(session_manager_)
    , shutdown_requested_(false) {}

Server::~Server() {
    stop();
}

bool Server::start() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    tcp_handler_ = std::make_unique<TcpHandler>(port_);
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
    shutdown_requested_ = true;
    event_loop_.stop();
    
    if (tcp_handler_) {
        tcp_handler_->stop();
    }
    if (udp_handler_) {
        udp_handler_->stop();
    }
    
    std::cout << "Server stopped" << std::endl;
}

void Server::run() {
    while (!shutdown_requested_ && !global_shutdown_flag) {
        event_loop_.run(1000); // 1 second timeout
    }
    stop();
}

void Server::setup_tcp_handler() {
    tcp_handler_->set_connection_callback([this](auto connection) {
        handle_tcp_connection(connection);
    });
    
    event_loop_.add_fd(tcp_handler_->get_socket_fd(), EPOLLIN, [this](uint32_t events) {
        if (events & EPOLLIN) {
            tcp_handler_->handle_accept();
        }
    });
}

void Server::setup_udp_handler() {
    udp_handler_->set_message_callback([this](const auto& message, const auto& client_addr) {
        handle_udp_message(message, client_addr);
    });
    
    event_loop_.add_fd(udp_handler_->get_socket_fd(), EPOLLIN, [this](uint32_t events) {
        if (events & EPOLLIN) {
            udp_handler_->handle_message();
        }
    });
}

void Server::handle_tcp_connection(std::shared_ptr<TcpConnection> connection) {
    session_manager_.add_connection(connection->get_fd());
    
    connection->set_message_callback([this, connection](const auto& message) {
        handle_tcp_message(message, connection);
    });
    
    connection->set_close_callback([this, connection]() {
        session_manager_.remove_connection(connection->get_fd());
        event_loop_.remove_fd(connection->get_fd());
    });
    
    event_loop_.add_fd(connection->get_fd(), EPOLLIN, [connection](uint32_t events) {
        if (events & EPOLLIN) {
            connection->handle_read();
        }
    });
}

void Server::handle_tcp_message(const std::string& message, std::shared_ptr<TcpConnection> connection) {
    std::string response = command_processor_.process_command(message);
    
    if (response == "/SHUTDOWN_ACK") {
        shutdown_requested_ = true;
        return;
    }
    
    response += "\n";
    connection->send(response);
    
    std::cout << "Sent TCP response to " << connection->get_client_info() 
              << ": " << response;
}

void Server::handle_udp_message(const std::string& message, const sockaddr_in& client_addr) {
    session_manager_.increment_total_connections();
    
    std::string response = command_processor_.process_command(message);
    
    if (response == "/SHUTDOWN_ACK") {
        shutdown_requested_ = true;
        return;
    }
    
    udp_handler_->send_message(response, client_addr);
    
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, ip, INET_ADDRSTRLEN);
    std::cout << "Sent UDP response to " << ip << ":" << ntohs(client_addr.sin_port)
              << ": " << response << std::endl;
}