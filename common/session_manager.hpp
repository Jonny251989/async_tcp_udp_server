#pragma once

#include <iostream>
#include <unordered_set>
#include <atomic>
#include <mutex>
#include <chrono>


struct ServerStats {
    size_t total_connections;
    size_t current_connections;
    std::chrono::system_clock::time_point start_time;
};

class SessionManager {
public:
    SessionManager();
    
    void add_connection();
    void remove_connection();
    void increment_total_connections();
    
    ServerStats get_stats() const;

private:
    std::atomic<size_t> total_connections_{0};
    std::atomic<size_t> current_connections_{0};
    std::chrono::system_clock::time_point start_time_;
};