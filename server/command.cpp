#include "command.hpp"
#include "session_manager.hpp"
#include <chrono>
#include <format>
#include <sstream>

std::string TimeCommand::execute() {
    auto zoned_time = std::chrono::zoned_time{"Europe/London", std::chrono::system_clock::now()};
    return std::format("{:%Y-%m-%d %H:%M:%S %Z}", zoned_time);
}

StatsCommand::StatsCommand(SessionManager& session_manager) 
    : session_manager_(session_manager) {}

std::string StatsCommand::execute() {
    auto stats = session_manager_.get_stats();
    std::ostringstream oss;
    oss << "Total connections: " << stats.total_connections << "\n"
        << "Current connections: " << stats.current_connections;
    return oss.str();
}

std::string ShutdownCommand::execute() {
    return "/SHUTDOWN_ACK";
}