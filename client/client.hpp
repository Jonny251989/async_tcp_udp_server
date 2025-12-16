#pragma once

#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>
#include <memory>

template <typename Derived>
class ClientBase {
protected:
    int sock_ = -1;
    sockaddr_in serv_addr_{};
    std::string server_ip_;
    uint16_t port_;

    Derived& derived() { return static_cast<Derived&>(*this); }
    const Derived& derived() const { return static_cast<const Derived&>(*this); }

public:
    ClientBase(const std::string& server_ip, uint16_t port)
        : server_ip_(server_ip), port_(port) {
    }

    ~ClientBase() {
        if (sock_ != -1) close(sock_);
    }

    void setup_socket() {
        sock_ = socket(AF_INET, Derived::PROTOCOL_TYPE, 0);
        if (sock_ < 0) {
            throw std::runtime_error("Socket creation failed");
        }

        std::memset(&serv_addr_, 0, sizeof(serv_addr_));
        serv_addr_.sin_family = AF_INET;
        serv_addr_.sin_port = htons(port_);

        if (inet_pton(AF_INET, server_ip_.c_str(), &serv_addr_.sin_addr) <= 0) {
            close(sock_);
            throw std::runtime_error("Invalid address: " + server_ip_);
        }
    }

    void send_message(const std::string& message) {
        ssize_t bytes_sent = derived().send_impl(message);
        if (bytes_sent < 0) {
            throw std::runtime_error("Send failed");
        }
    }

    std::string receive_response() {
        return derived().receive_impl();
    }

    void run() {
        std::cout << "Client started. Type your messages (type 'quit' or 'exit' to exit):" << std::endl;
        
        while (true) {
            std::cout << "> ";
            std::string message;
            
            if (!std::getline(std::cin, message)) {
                break; // EOF (Ctrl+D) 
            }
            
            if (message.empty()) {
                continue;
            }
            
            if (message == "quit" || message == "exit") {
                std::cout << "Exiting client..." << std::endl;
                break;
            }
            
            try {
                send_message(message + "\n");
        
                std::string response = receive_response();
                
                std::cout << response << std::endl;
                
            } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
                
                if (std::string(e.what()).find("closed") != std::string::npos ||
                    std::string(e.what()).find("failed") != std::string::npos) {
                    break;
                }
            }
        }
    }
};

class TcpClient : public ClientBase<TcpClient> {
public:
    static constexpr int PROTOCOL_TYPE = SOCK_STREAM;

    TcpClient(const std::string& server_ip, uint16_t port)
        : ClientBase(server_ip, port) {
        setup_socket();
        connect_to_server();
    }

    void connect_to_server() {
        if (connect(sock_, reinterpret_cast<sockaddr*>(&serv_addr_), sizeof(serv_addr_)) < 0) {
            close(sock_);
            throw std::runtime_error("Connection failed to " + server_ip_ + ":" + std::to_string(port_));
        }
        std::cout << "Connected to TCP server " << server_ip_ << ":" << port_ << std::endl;
    }

    ssize_t send_impl(const std::string& message) {
        return send(sock_, message.c_str(), message.length(), 0);
    }

    std::string receive_impl() {
        char buffer[1024] = {0};
        ssize_t bytes_read = recv(sock_, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            return std::string(buffer);
        } else if (bytes_read == 0) {
            throw std::runtime_error("Connection closed by server");
        } else {
            throw std::runtime_error("Receive failed");
        }
    }
};

class UdpClient : public ClientBase<UdpClient> {
public:
    static constexpr int PROTOCOL_TYPE = SOCK_DGRAM;

    UdpClient(const std::string& server_ip, uint16_t port)
        : ClientBase(server_ip, port) {
        setup_socket();
        std::cout << "UDP client ready to send to " << server_ip_ << ":" << port_ << std::endl;
    }

    ssize_t send_impl(const std::string& message) {
        return sendto(sock_, message.c_str(), message.length(), 0,
                     reinterpret_cast<sockaddr*>(&serv_addr_), sizeof(serv_addr_));
    }

    std::string receive_impl() {
        char buffer[1024] = {0};
        sockaddr_in from_addr{};
        socklen_t addr_len = sizeof(from_addr);
        
        ssize_t bytes_read = recvfrom(sock_, buffer, sizeof(buffer) - 1, 0,
                                    reinterpret_cast<sockaddr*>(&from_addr), &addr_len);
        
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            return std::string(buffer);
        } else {
            throw std::runtime_error("Receive failed");
        }
    }
};