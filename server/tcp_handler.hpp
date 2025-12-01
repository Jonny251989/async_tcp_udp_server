#pragma once

#pragma once

#include "tcp_connection.hpp"
#include <memory>
#include <unordered_map>
#include <functional>
#include <arpa/inet.h>

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

class TcpHandler {
public:
    TcpHandler(uint16_t port, std::shared_ptr<SessionManager> session_manager);
    ~TcpHandler();
    
    bool start();
    void stop();
    void handle_accept();
    int get_socket_fd() const { return socket_fd_; }
    
    void set_connection_callback(std::function<void(std::shared_ptr<TcpConnection>)> callback) {
        connection_callback_ = std::move(callback);
    }

private:
    uint16_t port_;
    int socket_fd_;
    std::shared_ptr<SessionManager> session_manager_;
    std::unordered_map<int, std::shared_ptr<TcpConnection>> connections_;
    std::function<void(std::shared_ptr<TcpConnection>)> connection_callback_;
};