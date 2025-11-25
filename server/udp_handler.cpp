#include "udp_handler.hpp"

UdpHandler::UdpHandler(uint16_t port) : port_(port), socket_fd_(-1) {}

UdpHandler::~UdpHandler() {
    stop();
}

bool UdpHandler::start() {
    socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd_ == -1) {
        return false;
    }
    
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(socket_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(socket_fd_);
        return false;
    }
    
    int flags = fcntl(socket_fd_, F_GETFL, 0);
    if (flags == -1 || fcntl(socket_fd_, F_SETFL, flags | O_NONBLOCK) == -1) {
        close(socket_fd_);
        return false;
    }
    
    std::cout << "UDP handler started on port " << port_ << std::endl;
    return true;
}

void UdpHandler::stop() {
    if (socket_fd_ != -1) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
}

void UdpHandler::send_message(const std::string& message, const sockaddr_in& client_addr) {
    if (socket_fd_ != -1) {
        sendto(socket_fd_, message.c_str(), message.length(), 0,
               reinterpret_cast<const sockaddr*>(&client_addr), sizeof(client_addr));
    }
}

void UdpHandler::handle_message() {
    char buffer[1024];
    sockaddr_in client_addr{};
    socklen_t addr_len = sizeof(client_addr);
    
    ssize_t bytes_read = recvfrom(socket_fd_, buffer, sizeof(buffer) - 1, 0,
                                reinterpret_cast<sockaddr*>(&client_addr), &addr_len);
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        std::string message(buffer);
        message.erase(message.find_last_not_of(" \t\n\r\f\v") + 1);
        
        if (message_callback_) {
            message_callback_(message, client_addr);
        }
        
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, INET_ADDRSTRLEN);
        std::cout << "UDP message from " << ip << ":" << ntohs(client_addr.sin_port) 
                  << ": " << message << std::endl;
    }
}