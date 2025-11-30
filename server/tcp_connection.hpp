#pragma once

#include "session_manager.hpp"
#include <memory>
#include <functional>
#include <string>
#include <netinet/in.h>

class TcpConnection {
public:
    using MessageCallback = std::function<void(const std::string&)>;
    using CloseCallback = std::function<void()>;
    
    TcpConnection(int fd, const sockaddr_in& client_addr, std::shared_ptr<SessionManager> session_manager);
    ~TcpConnection();
    
    void send(const std::string& message);
    void close();
    void handle_read();
    
    int get_fd() const { return fd_; }
    std::string get_client_info() const;
    
    void set_message_callback(MessageCallback callback) {
        message_callback_ = std::move(callback);
    }
    
    void set_close_callback(CloseCallback callback) {
        close_callback_ = std::move(callback);
    }

private:
    static const int BUFFER_SIZE = 4096;
    
    int fd_;
    sockaddr_in client_addr_;
    std::shared_ptr<SessionManager> session_manager_;
    MessageCallback message_callback_;
    CloseCallback close_callback_;
};