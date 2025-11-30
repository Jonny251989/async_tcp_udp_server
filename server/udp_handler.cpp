#include "udp_handler.hpp"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

UdpHandler::UdpHandler(uint16_t port) 
    : port_(port), socket_fd_(-1) {
    std::cout << "UdpHandler constructor: port=" << port_ << std::endl;
}

UdpHandler::~UdpHandler() {
    std::cout << "UdpHandler destructor" << std::endl;
    stop();
}

bool UdpHandler::start() {
    std::cout << "=== UDP HANDLER START ===" << std::endl;
    
    // 1. Создание UDP сокета
    std::cout << "1. Creating UDP socket..." << std::endl;
    socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd_ == -1) {
        std::cerr << "❌ UDP socket() failed: " << strerror(errno) << std::endl;
        return false;
    }
    std::cout << "✓ UDP socket created, fd: " << socket_fd_ << std::endl;
    
    // 2. Привязка к адресу
    std::cout << "2. Binding UDP to port " << port_ << "..." << std::endl;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(socket_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "❌ UDP bind() failed on port " << port_ << ": " << strerror(errno) << std::endl;
        
        // Проверим права
        if (port_ < 1024) {
            std::cerr << "💡 Note: Ports below 1024 require root privileges" << std::endl;
        }
        
        close(socket_fd_);
        return false;
    }
    std::cout << "✓ UDP bind successful" << std::endl;
    
    // 3. Установка non-blocking режима
    std::cout << "3. Setting UDP non-blocking mode..." << std::endl;
    int flags = fcntl(socket_fd_, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "❌ UDP fcntl(F_GETFL) failed: " << strerror(errno) << std::endl;
        close(socket_fd_);
        return false;
    }
    
    if (fcntl(socket_fd_, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "❌ UDP fcntl(F_SETFL) failed: " << strerror(errno) << std::endl;
        close(socket_fd_);
        return false;
    }
    std::cout << "✓ UDP non-blocking mode set" << std::endl;
    
    std::cout << "✅ UDP handler started successfully on port " << port_ << std::endl;
    std::cout << "=== UDP HANDLER READY ===" << std::endl;
    return true;
}

void UdpHandler::stop() {
    std::cout << "UdpHandler::stop() - closing socket fd: " << socket_fd_ << std::endl;
    if (socket_fd_ != -1) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
}

void UdpHandler::handle_message() {
    std::cout << "UdpHandler::handle_message() - waiting for data..." << std::endl;
    
    char buffer[1024];
    sockaddr_in client_addr{};
    socklen_t addr_len = sizeof(client_addr);
    
    ssize_t bytes_read = recvfrom(socket_fd_, buffer, sizeof(buffer) - 1, 0,
                                reinterpret_cast<sockaddr*>(&client_addr), &addr_len);
    
    std::cout << "UdpHandler: recvfrom returned: " << bytes_read << " bytes" << std::endl;
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        std::string message(buffer);
        message.erase(message.find_last_not_of(" \t\n\r\f\v") + 1);
        
        std::cout << "UdpHandler: Received message: '" << message << "'" << std::endl;
        
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, INET_ADDRSTRLEN);
        std::cout << "UdpHandler: From " << ip << ":" << ntohs(client_addr.sin_port) << std::endl;
        
        if (message_callback_) {
            std::cout << "UdpHandler: Calling message_callback..." << std::endl;
            message_callback_(message, client_addr);
        } else {
            std::cout << "UdpHandler: NO message_callback set!" << std::endl;
        }
    } else if (bytes_read == 0) {
        std::cout << "UdpHandler: recvfrom returned 0 bytes" << std::endl;
    } else {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cout << "UdpHandler: recvfrom error: " << strerror(errno) << std::endl;
        } else {
            std::cout << "UdpHandler: No data available (non-blocking)" << std::endl;
        }
    }
}

void UdpHandler::send_message(const std::string& message, const sockaddr_in& client_addr) {
    std::cout << "UdpHandler::send_message() - sending: '" << message << "'" << std::endl;
    
    if (socket_fd_ != -1) {
        ssize_t bytes_sent = sendto(socket_fd_, message.c_str(), message.length(), 0,
                                  reinterpret_cast<const sockaddr*>(&client_addr), sizeof(client_addr));
        
        if (bytes_sent > 0) {
            std::cout << "UdpHandler: Sent " << bytes_sent << " bytes" << std::endl;
        } else {
            std::cout << "UdpHandler: sendto failed: " << strerror(errno) << std::endl;
        }
    } else {
        std::cout << "UdpHandler: Cannot send - socket not available" << std::endl;
    }
}