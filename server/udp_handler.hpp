#pragma once

#include <functional>
#include <string>
#include <netinet/in.h>
#include <arpa/inet.h>  
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

class UdpHandler {
public:
    UdpHandler(uint16_t port);
    ~UdpHandler();
    
    bool start();
    void stop();
    void handle_message();
    void send_message(const std::string& message, const sockaddr_in& client_addr);
    int get_socket_fd() const { return socket_fd_; }
    
    void set_message_callback(std::function<void(const std::string&, const sockaddr_in&)> callback) {
        message_callback_ = std::move(callback);
    }

private:
    uint16_t port_;
    int socket_fd_;
    std::function<void(const std::string&, const sockaddr_in&)> message_callback_;
};