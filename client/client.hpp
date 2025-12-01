#pragma once

#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>

template <int TYPE>
class Client {
public:
    Client(const std::string& server_ip, uint16_t port);
    ~Client();
    
    void run();

private:
    void setup_socket();
    void connect_to_server();
    void send_message(const std::string& message);
    std::string receive_response();
    bool is_interactive() const { return isatty(STDIN_FILENO); }
    
    std::string server_ip_;
    uint16_t port_;
    int sock_;
    sockaddr_in serv_addr_;
};

template <int TYPE>
Client<TYPE>::Client(const std::string& server_ip, uint16_t port) 
    : server_ip_(server_ip), port_(port), sock_(-1) {
    setup_socket();
    connect_to_server();
}

template <int TYPE>
Client<TYPE>::~Client() {
    if (sock_ != -1) close(sock_);
}

template <int TYPE>
void Client<TYPE>::setup_socket() {
    sock_ = socket(AF_INET, TYPE, 0);
    if (sock_ < 0) throw std::runtime_error("Socket creation failed");
    
    std::memset(&serv_addr_, 0, sizeof(serv_addr_));
    serv_addr_.sin_family = AF_INET;
    serv_addr_.sin_port = htons(port_);
    
    if (inet_pton(AF_INET, server_ip_.c_str(), &serv_addr_.sin_addr) <= 0) {
        close(sock_);
        throw std::runtime_error("Invalid address: " + server_ip_);
    }
}

template <int TYPE>
void Client<TYPE>::connect_to_server() {
    if constexpr (TYPE == SOCK_STREAM) {
        if (connect(sock_, (sockaddr*)&serv_addr_, sizeof(serv_addr_)) < 0) {
            close(sock_);
            throw std::runtime_error("Connection failed to " + server_ip_ + ":" + std::to_string(port_));
        }
        std::cout << "Connected to TCP server " << server_ip_ << ":" << port_ << std::endl;
    } else {
        std::cout << "UDP client ready to send to " << server_ip_ << ":" << port_ << std::endl;
    }
}

template <int TYPE>
void Client<TYPE>::send_message(const std::string& message) {
    ssize_t bytes_sent;
    
    if constexpr (TYPE == SOCK_STREAM) {
        bytes_sent = send(sock_, message.c_str(), message.length(), 0);
    } else {
        bytes_sent = sendto(sock_, message.c_str(), message.length(), 0,
                           (sockaddr*)&serv_addr_, sizeof(serv_addr_));
    }
    
    if (bytes_sent < 0) throw std::runtime_error("Send failed");
}

template <int TYPE>
std::string Client<TYPE>::receive_response() {
    char buffer[1024] = {0};
    ssize_t bytes_read;
    
    if constexpr (TYPE == SOCK_STREAM) {
        bytes_read = recv(sock_, buffer, sizeof(buffer) - 1, 0);
    } else {
        sockaddr_in from_addr{};
        socklen_t addr_len = sizeof(from_addr);
        bytes_read = recvfrom(sock_, buffer, sizeof(buffer) - 1, 0,
                            (sockaddr*)&from_addr, &addr_len);
    }
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        return std::string(buffer);
    } else if (bytes_read == 0) {
        throw std::runtime_error("Connection closed by server");
    } else {
        throw std::runtime_error("Receive failed");
    }
}

template <int TYPE>
void Client<TYPE>::run() {
    if (!is_interactive()) {
        // Pipe mode - читаем из stdin до конца
        std::string message;
        while (std::getline(std::cin, message)) {
            if (message.empty()) continue;
            
            try {
                send_message(message + "\n");
                std::string response = receive_response();
                std::cout << response << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
                break;
            }
        }
    } else {
        // Interactive mode
        std::cout << "Client started. Type your messages (type 'quit' to exit):" << std::endl;
        
        while (true) {
            std::cout << "Enter message: ";
            std::string message;
            if (!std::getline(std::cin, message)) {
                break; // EOF или ошибка ввода
            }
            
            if (message == "quit" || message == "exit") {
                std::cout << "Exiting client..." << std::endl;
                break;
            }
            
            if (message.empty()) {
                std::cout << "Message cannot be empty. Try again." << std::endl;
                continue;
            }
            
            try {
                send_message(message + "\n");
                std::string response = receive_response();
                std::cout << "Server response: " << response << std::endl;
                
                std::cout << "Continue? (y/n): ";
                std::string answer;
                if (!std::getline(std::cin, answer)) {
                    break;
                }
                
                if (answer == "n" || answer == "N" || answer == "no") {
                    std::cout << "Exiting client..." << std::endl;
                    break;
                }
                
            } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
                break;
            }
        }
    }
}

using TcpClient = Client<SOCK_STREAM>;
using UdpClient = Client<SOCK_DGRAM>;