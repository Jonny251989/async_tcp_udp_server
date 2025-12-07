#include "client.hpp"
#include <iostream>
#include <string>

#include "client.hpp"
#include <iostream>
#include <string>

void print_usage() {
    std::cout << "Usage: client <protocol> <server_ip> <port>" << std::endl;
    std::cout << "Arguments:" << std::endl;
    std::cout << "  protocol  - tcp or udp" << std::endl;
    std::cout << "  server_ip - IP address of the server" << std::endl;
    std::cout << "  port      - port number" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  client tcp 127.0.0.1 8080" << std::endl;
    std::cout << "  client udp 127.0.0.1 8080" << std::endl;
    std::cout << std::endl;
    std::cout << "Once connected, you can send messages interactively." << std::endl;
    std::cout << "Type 'quit' to exit or answer 'n' when asked to continue." << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        print_usage();
        return 1;
    }

    std::string protocol = argv[1];
    std::string server_ip = argv[2];
    uint16_t port;
    
    try {
        port = static_cast<uint16_t>(std::stoi(argv[3]));
    } catch (const std::exception& e) {
        std::cerr << "Error: Invalid port number" << std::endl;
        print_usage();
        return 1;
    }

    try {
        if (protocol == "tcp") {
            TcpClient client(server_ip, port);
            client.run();
        } else if (protocol == "udp") {
            UdpClient client(server_ip, port);
            client.run();
        } else {
            std::cerr << "Error: Invalid protocol. Use 'tcp' or 'udp'" << std::endl;
            print_usage();
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}