#pragma once

#include "../common/protocol.hpp"
#include <iostream>
#include <unordered_set>
#include <atomic>
#include <mutex>

class SessionManager {
public:
    SessionManager();
    
    void add_connection(int fd);
    void remove_connection(int fd);
    void increment_total_connections();
    
    ServerStats get_stats() const;

private:
    std::atomic<size_t> total_connections_{0};
    std::atomic<size_t> current_connections_{0};
    std::chrono::system_clock::time_point start_time_;
    
    mutable std::mutex connections_mutex_;
    std::unordered_set<int> active_connections_;
};