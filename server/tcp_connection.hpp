#pragma once

#include "session_manager.hpp"
#include <memory>
#include <functional>
#include <string>
#include <netinet/in.h>

#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
    TcpConnection(int fd, const sockaddr_in& client_addr, std::shared_ptr<SessionManager> session_manager);
    ~TcpConnection();
    
    void send(const std::string& message);
    void close();
    void handle_read();
    int get_fd() const { return fd_; }
    std::string get_client_info() const;
    
    void set_message_callback(std::function<void(const std::string&)> callback) {
        message_callback_ = std::move(callback);
    }
    
    void set_close_callback(std::function<void()> callback) {
        close_callback_ = std::move(callback);
    }

private:
    static const size_t BUFFER_SIZE = 1024;
    
    int fd_;
    sockaddr_in client_addr_;
    std::shared_ptr<SessionManager> session_manager_;
    std::function<void(const std::string&)> message_callback_;
    std::function<void()> close_callback_;
};