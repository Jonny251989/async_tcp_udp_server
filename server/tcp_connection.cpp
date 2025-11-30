#include "tcp_connection.hpp"
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

TcpConnection::TcpConnection(int fd, const sockaddr_in& client_addr, std::shared_ptr<SessionManager> session_manager)
    : fd_(fd)
    , client_addr_(client_addr)
    , session_manager_(session_manager) {
    
    if (session_manager_) {
        session_manager_->add_connection();
    }
}

TcpConnection::~TcpConnection() {
    if (session_manager_) {
        session_manager_->remove_connection();
    }
    close();
}

void TcpConnection::send(const std::string& message) {
    if (fd_ != -1) {
        ::send(fd_, message.c_str(), message.length(), 0);
    }
}

void TcpConnection::close() {
    if (fd_ != -1) {
        ::close(fd_);
        fd_ = -1;
        if (close_callback_) {
            close_callback_();
        }
    }
}

std::string TcpConnection::get_client_info() const {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr_.sin_addr, ip, INET_ADDRSTRLEN);
    return std::string(ip) + ":" + std::to_string(ntohs(client_addr_.sin_port));
}

void TcpConnection::handle_read() {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = recv(fd_, buffer, BUFFER_SIZE - 1, 0);
    
    std::cout << "TcpConnection::handle_read() - bytes_read: " << bytes_read << std::endl;
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        std::string message(buffer);
        
        // Убираем лишние пробелы и переводы строк
        message.erase(message.find_last_not_of(" \t\n\r\f\v") + 1);
        
        std::cout << "TcpConnection: Received message: '" << message << "'" << std::endl;
        std::cout << "TcpConnection: Message length: " << message.length() << std::endl;
        
        if (message_callback_) {
            std::cout << "TcpConnection: Calling message_callback..." << std::endl;
            message_callback_(message);
        } else {
            std::cout << "TcpConnection: NO message_callback set!" << std::endl;
        }
    } else if (bytes_read == 0) {
        std::cout << "TcpConnection: Connection closed by client" << std::endl;
        close();
    } else {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cout << "TcpConnection: recv error: " << strerror(errno) << std::endl;
        }
    }
}