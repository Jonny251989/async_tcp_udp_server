#include "command.hpp"

std::string TimeCommand::execute() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    // Получаем локальное время
    std::tm local_tm{};
    localtime_r(&time_t, &local_tm);
    
    // Форматируем время вручную
    std::ostringstream oss;
    oss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
    
    // Добавляем информацию о временной зоне
    std::string timezone = "UTC";
    char tz_buf[64];
    if (std::strftime(tz_buf, sizeof(tz_buf), "%Z", &local_tm) > 0) {
        timezone = tz_buf;
    }
    
    oss << " " << timezone;
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
    std::cout << "!!! SHUTDOWN COMMAND EXECUTE CALLED !!!" << std::endl;
    std::cout << "Returning /SHUTDOWN_ACK" << std::endl;
    return "/SHUTDOWN_ACK";
}