#include "server.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    uint16_t port = 0;
    
    // Получаем порт из переменной окружения или аргументов командной строки
    char* env_port = std::getenv("SERVER_PORT");
    if (env_port != nullptr) {
        try {
            port = static_cast<uint16_t>(std::stoi(env_port));
            std::cout << "Using port from environment: " << port << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error: Invalid SERVER_PORT environment variable" << std::endl;
            return 1;
        }
    } else if (argc == 2) {
        try {
            port = static_cast<uint16_t>(std::stoi(argv[1]));
            std::cout << "Using port from command line: " << port << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error: Invalid port number" << std::endl;
            return 1;
        }
    } else {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        std::cerr << "Or set SERVER_PORT environment variable" << std::endl;
        return 1;
    }

    try {
        Server server(port);
        
        if (!server.start()) {
            std::cerr << "Failed to start server" << std::endl;
            return 1;
        }
        
        std::cout << "Server running on port " << port << std::endl;
        std::cout << "Press Ctrl+C to exit..." << std::endl;
        
        server.run();
        
        std::cout << "Server stopped gracefully" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}