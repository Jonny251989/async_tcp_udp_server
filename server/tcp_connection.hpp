#pragma once

#include <string>
#include <functional>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

class TcpConnection {
public:
    using MessageCallback = std::function<void(const std::string&)>;
    using CloseCallback = std::function<void()>;
    
    TcpConnection(int fd, const sockaddr_in& client_addr);
    ~TcpConnection();
    
    void set_message_callback(MessageCallback callback) { message_callback_ = std::move(callback); }
    void set_close_callback(CloseCallback callback) { close_callback_ = std::move(callback); }
    
    void send(const std::string& message);
    void close();
    int get_fd() const { return fd_; }
    std::string get_client_info() const;
    void handle_read();

private:
    int fd_;
    sockaddr_in client_addr_;
    MessageCallback message_callback_;
    CloseCallback close_callback_;
    static constexpr int BUFFER_SIZE = 1024;
};