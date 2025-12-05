#pragma once

#include "eventloop.hpp"
#include "tcp_handler.hpp"
#include "udp_handler.hpp"
#include "command_processor.hpp"
#include "session_manager.hpp"
#include "command.hpp"
#include <memory>
#include <atomic>
#include <csignal>
#include <vector>
#include <iostream>
#include <netinet/in.h>
#include <thread>     
#include <chrono>     

#include <sys/epoll.h>

extern std::atomic<bool> global_shutdown_flag;

class Server {
public:
    Server(uint16_t port);
    ~Server();
    
    bool start();
    void run();
    void stop();

private:
    std::vector<std::unique_ptr<Command>> create_commands();
    void setup_tcp_handler();
    void setup_udp_handler();
    void handle_tcp_connection(std::shared_ptr<TcpConnection> connection);
    void handle_tcp_message(const std::string& message, std::shared_ptr<TcpConnection> connection);
    void handle_udp_message(const std::string& message, const sockaddr_in& client_addr);

    uint16_t port_;
    std::unique_ptr<TcpHandler> tcp_handler_;
    std::unique_ptr<UdpHandler> udp_handler_;
    EventLoop event_loop_;
    std::shared_ptr<SessionManager> session_manager_;
    CommandProcessor command_processor_;
    bool shutdown_requested_;
};