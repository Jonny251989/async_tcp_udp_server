#pragma once

#include <cstdint>
#include <string>
#include <chrono>

#include <sstream>
#include <iomanip>
struct ServerStats {
    size_t total_connections;
    size_t current_connections;
    std::chrono::system_clock::time_point start_time;
};

std::string get_current_time_string();