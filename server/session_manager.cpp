#include "session_manager.hpp"

SessionManager::SessionManager() 
    : start_time_(std::chrono::system_clock::now()) {}

void SessionManager::add_connection() {
    total_connections_++;
    current_connections_++;
}

void SessionManager::remove_connection() {
    current_connections_--;
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