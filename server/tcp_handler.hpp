#pragma once

#include "tcp_connection.hpp"
#include <memory>
#include <unordered_map>
#include <functional>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>

class TcpHandler {
public:
    using ConnectionCallback = std::function<void(std::shared_ptr<TcpConnection>)>;
    
    TcpHandler(uint16_t port);
    ~TcpHandler();
    
    bool start();
    void stop();
    int get_socket_fd() const { return socket_fd_; }
    
    void set_connection_callback(ConnectionCallback callback) { connection_callback_ = std::move(callback); }
    void handle_accept();

private:
    uint16_t port_;
    int socket_fd_;
    ConnectionCallback connection_callback_;
    std::unordered_map<int, std::shared_ptr<TcpConnection>> connections_;
};