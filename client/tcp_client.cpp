#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

void print_usage() {
    std::cout << "Usage: tcp_client <server_ip> <port> <message>" << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  tcp_client 127.0.0.1 8080 \"Hello World\"" << std::endl;
    std::cout << "  tcp_client 127.0.0.1 8080 \"/time\"" << std::endl;
    std::cout << "  tcp_client 127.0.0.1 8080 \"/stats\"" << std::endl;
    std::cout << "  tcp_client 127.0.0.1 8080 \"/shutdown\"" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        print_usage();
        return 1;
    }

    std::string server_ip = argv[1];
    uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));
    std::string message = argv[3];

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return 1;
    }

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, server_ip.c_str(), &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address" << std::endl;
        close(sock);
        return 1;
    }

    if (connect(sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        close(sock);
        return 1;
    }

    // Send message
    ssize_t bytes_sent = send(sock, message.c_str(), message.length(), 0);
    if (bytes_sent < 0) {
        std::cerr << "Send failed" << std::endl;
        close(sock);
        return 1;
    }
    
    // Receive response
    char buffer[1024] = {0};
    ssize_t bytes_read = recv(sock, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        std::cout << "Server response: " << buffer << std::endl;
    } else {
        std::cerr << "Receive failed or connection closed" << std::endl;
    }
    
    close(sock);
    return 0;
}