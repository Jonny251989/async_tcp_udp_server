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
    
    TcpHandler(uint16_t port, std::shared_ptr<SessionManager> session_manager);
    ~TcpHandler();
    
    bool start();
    void stop();
    void handle_accept();
    int get_socket_fd() const { return socket_fd_; }
    
    void set_connection_callback(ConnectionCallback callback) {
        connection_callback_ = std::move(callback);
    }

private:
    uint16_t port_;
    std::shared_ptr<SessionManager> session_manager_;
    int socket_fd_;
    std::unordered_map<int, std::shared_ptr<TcpConnection>> connections_;
    ConnectionCallback connection_callback_;
};