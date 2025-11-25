#include "command_processor.hpp"

CommandProcessor::CommandProcessor(SessionManager& stats) : stats_(stats) {
    register_command("time", [this]() { return handle_time(); });
    register_command("stats", [this]() { return handle_stats(); });
    register_command("shutdown", [this]() { return handle_shutdown(); });
}

void CommandProcessor::register_command(const std::string& name, CommandHandler handler) {
    commands_["/" + name] = std::move(handler);
}

std::string CommandProcessor::process_command(const std::string& command) {
    if (!is_command(command)) {
        return handle_mirror(command);
    }
    
    auto it = commands_.find(command);
    if (it != commands_.end()) {
        return it->second();
    }
    
    return "ERROR: Unknown command";
}

std::string CommandProcessor::handle_time() {
    return get_current_time_string();
}

std::string CommandProcessor::handle_stats() {
    auto stats = stats_.get_stats();
    std::ostringstream oss;
    oss << "Total connections: " << stats.total_connections << "\n"
        << "Current connections: " << stats.current_connections;
    return oss.str();
}

std::string CommandProcessor::handle_shutdown() {
    return "/SHUTDOWN_ACK";
}

std::string CommandProcessor::handle_mirror(const std::string& message) {
    return message;
}

bool CommandProcessor::is_command(const std::string& message) {
    return !message.empty() && message[0] == '/';
}