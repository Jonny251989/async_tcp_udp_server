#include "tcp_handler.hpp"


TcpHandler::TcpHandler(uint16_t port, std::shared_ptr<SessionManager> session_manager) 
    : port_(port), socket_fd_(-1), session_manager_(session_manager) {}

TcpHandler::~TcpHandler() {
    stop();
}

bool TcpHandler::start() {
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ == -1) return false;
    
    int opt = 1;
    if (setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(socket_fd_);
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
    
    if (listen(socket_fd_, 128) < 0) {
        close(socket_fd_);
        return false;
    }
    
    int flags = fcntl(socket_fd_, F_GETFL, 0);
    if (flags == -1 || fcntl(socket_fd_, F_SETFL, flags | O_NONBLOCK) == -1) {
        close(socket_fd_);
        return false;
    }
    
    return true;
}

void TcpHandler::stop() {
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
    
    if (client_fd != -1) {
        int flags = fcntl(client_fd, F_GETFL, 0);
        if (flags != -1) {
            fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
        }
        
        auto connection = std::make_shared<TcpConnection>(client_fd, client_addr, session_manager_);
        connections_[client_fd] = connection;
        
        if (connection_callback_) {
            connection_callback_(connection);
        }
    }
}