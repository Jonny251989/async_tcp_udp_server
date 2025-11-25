#pragma once

#include <iostream>
#include <cstring>
#include <string>
#include <functional>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>

class UdpHandler {
public:
    using MessageCallback = std::function<void(const std::string&, const sockaddr_in&)>;
    
    UdpHandler(uint16_t port);
    ~UdpHandler();
    
    bool start();
    void stop();
    int get_socket_fd() const { return socket_fd_; }
    
    void set_message_callback(MessageCallback callback) { message_callback_ = std::move(callback); }
    void send_message(const std::string& message, const sockaddr_in& client_addr);
    void handle_message();

private:
    uint16_t port_;
    int socket_fd_;
    MessageCallback message_callback_;
};