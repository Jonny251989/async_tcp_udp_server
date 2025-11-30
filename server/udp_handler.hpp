#pragma once

#include <functional>
#include <string>
#include <netinet/in.h>
#include <arpa/inet.h>  


class UdpHandler {
public:
    using MessageCallback = std::function<void(const std::string&, const sockaddr_in&)>;
    
    UdpHandler(uint16_t port);
    ~UdpHandler();
    
    bool start();
    void stop();
    void handle_message();
    void send_message(const std::string& message, const sockaddr_in& client_addr);
    
    void set_message_callback(MessageCallback callback) {
        message_callback_ = std::move(callback);
    }
    
    int get_socket_fd() const { return socket_fd_; }

private:
    uint16_t port_;
    int socket_fd_;
    MessageCallback message_callback_;
};