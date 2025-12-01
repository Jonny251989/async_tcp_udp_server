#include "command.hpp"

std::string TimeCommand::execute() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::tm local_tm{};
    localtime_r(&time_t, &local_tm);
    
    std::ostringstream oss;
    oss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
    
    return oss.str();
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