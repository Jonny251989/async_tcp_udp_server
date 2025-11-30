#pragma once

#pragma once

#include "tcp_connection.hpp"
#include <memory>
#include <unordered_map>
#include <functional>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <arpa/inet.h> 

class TcpHandler {
public:
    using ConnectionCallback = std::function<void(std::shared_ptr<TcpConnection>)>;
    
    TcpHandler(uint16_t port, std::shared_ptr<SessionManager> session_manager);
    ~TcpHandler();
    
    bool start();
    void stop();
    void handle_accept();
    
    void set_connection_callback(ConnectionCallback callback) {
        connection_callback_ = std::move(callback);
    }
    
    int get_socket_fd() const { return socket_fd_; }

private:

    uint16_t port_;
    int socket_fd_;
    std::shared_ptr<SessionManager> session_manager_;
    ConnectionCallback connection_callback_;
    std::unordered_map<int, std::shared_ptr<TcpConnection>> connections_;
};