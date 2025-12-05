#include "tcp_connection.hpp"

TcpConnection::TcpConnection(int fd, const sockaddr_in& client_addr, std::shared_ptr<SessionManager> session_manager)
    : fd_(fd)
    , client_addr_(client_addr)
    , session_manager_(session_manager) 
{
    if (session_manager_) {
        session_manager_->add_connection();
    }
}

TcpConnection::~TcpConnection() {
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
        
        if (session_manager_) {
            session_manager_->remove_connection();
        }
        
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
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        std::string message(buffer);
        message.erase(message.find_last_not_of(" \t\n\r\f\v") + 1);
        
        if (message_callback_) {
            message_callback_(message);
        }
    } else if (bytes_read == 0) {
        close();
    }
}