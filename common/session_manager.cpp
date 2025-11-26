#include "session_manager.hpp"


SessionManager::SessionManager() 
    : start_time_(std::chrono::system_clock::now()) {}

void SessionManager::add_connection(int fd) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    active_connections_.insert(fd);
    total_connections_++;
    current_connections_ = active_connections_.size();
    
    std::cout << "New connection. Total: " << total_connections_.load() 
              << ", Current: " << current_connections_.load() << std::endl;
}

void SessionManager::remove_connection(int fd) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    active_connections_.erase(fd);
    current_connections_ = active_connections_.size();
    
    std::cout << "Client disconnected. Current connections: " 
              << current_connections_.load() << std::endl;
}

void SessionManager::increment_total_connections() {
    total_connections_++;
}

ServerStats SessionManager::get_stats() const {
    ServerStats stats{};
    stats.total_connections = total_connections_.load();
    stats.current_connections = current_connections_.load();
    stats.start_time = start_time_;
    return stats;
}