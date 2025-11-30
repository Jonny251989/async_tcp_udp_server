#include "tcp_handler.hpp"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

TcpHandler::TcpHandler(uint16_t port, std::shared_ptr<SessionManager> session_manager) 
    : port_(port), socket_fd_(-1), session_manager_(session_manager) {
    std::cout << "TcpHandler constructor: port=" << port_ << std::endl;
}

TcpHandler::~TcpHandler() {
    std::cout << "TcpHandler destructor" << std::endl;
    stop();
}

bool TcpHandler::start() {
    std::cout << "=== TCP HANDLER START ===" << std::endl;
    
    // 1. Создание сокета
    std::cout << "1. Creating socket..." << std::endl;
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ == -1) {
        std::cerr << "❌ socket() failed: " << strerror(errno) << std::endl;
        return false;
    }
    std::cout << "✓ Socket created, fd: " << socket_fd_ << std::endl;
    
    // 2. Установка SO_REUSEADDR
    std::cout << "2. Setting SO_REUSEADDR..." << std::endl;
    int opt = 1;
    if (setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "❌ setsockopt(SO_REUSEADDR) failed: " << strerror(errno) << std::endl;
        close(socket_fd_);
        return false;
    }
    std::cout << "✓ SO_REUSEADDR set" << std::endl;
    
    // 3. Привязка к адресу
    std::cout << "3. Binding to port " << port_ << "..." << std::endl;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(socket_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "❌ bind() failed on port " << port_ << ": " << strerror(errno) << std::endl;
        
        // Проверим права
        if (port_ < 1024) {
            std::cerr << "💡 Note: Ports below 1024 require root privileges" << std::endl;
        }
        
        close(socket_fd_);
        return false;
    }
    std::cout << "✓ Bind successful" << std::endl;
    
    // 4. Прослушивание
    std::cout << "4. Starting listen..." << std::endl;
    if (listen(socket_fd_, 128) < 0) {
        std::cerr << "❌ listen() failed: " << strerror(errno) << std::endl;
        close(socket_fd_);
        return false;
    }
    std::cout << "✓ Listen successful" << std::endl;
    
    // 5. Установка non-blocking режима
    std::cout << "5. Setting non-blocking mode..." << std::endl;
    int flags = fcntl(socket_fd_, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "❌ fcntl(F_GETFL) failed: " << strerror(errno) << std::endl;
        close(socket_fd_);
        return false;
    }
    
    if (fcntl(socket_fd_, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "❌ fcntl(F_SETFL) failed: " << strerror(errno) << std::endl;
        close(socket_fd_);
        return false;
    }
    std::cout << "✓ Non-blocking mode set" << std::endl;
    
    std::cout << "✅ TCP handler started successfully on port " << port_ << std::endl;
    std::cout << "=== TCP HANDLER READY ===" << std::endl;
    return true;
}

void TcpHandler::stop() {
    std::cout << "TcpHandler::stop() - closing socket fd: " << socket_fd_ << std::endl;
    if (socket_fd_ != -1) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
    connections_.clear();
}

void TcpHandler::handle_accept() {
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(socket_fd_, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
    
    std::cout << "TcpHandler::handle_accept() - client_fd: " << client_fd << std::endl;
    
    if (client_fd != -1) {
        // УДАЛИ ЭТУ СТРОКУ:
        // handle_immediate_read(client_fd);
        
        // Настраиваем non-blocking режим
        int flags = fcntl(client_fd, F_GETFL, 0);
        if (flags != -1) {
            fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
        }
        
        auto connection = std::make_shared<TcpConnection>(client_fd, client_addr, session_manager_);
        connections_[client_fd] = connection;
        
        std::cout << "TcpHandler: New connection created" << std::endl;
        
        if (connection_callback_) {
            std::cout << "TcpHandler: Calling connection_callback..." << std::endl;
            connection_callback_(connection);
        }
        
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, INET_ADDRSTRLEN);
        std::cout << "TcpHandler: New TCP connection from " << ip << ":" << ntohs(client_addr.sin_port) << std::endl;
    }
}
