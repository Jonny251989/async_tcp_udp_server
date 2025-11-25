#pragma once

#include "../common/protocol.hpp"

#include <sstream>
#include "session_manager.hpp"
#include <string>
#include <functional>
#include <unordered_map>

class CommandProcessor {
public:
    using CommandHandler = std::function<std::string()>;
    
    CommandProcessor(SessionManager& stats);
    
    std::string process_command(const std::string& command);
    void register_command(const std::string& name, CommandHandler handler);

private:
    SessionManager& stats_;
    std::unordered_map<std::string, CommandHandler> commands_;
    
    std::string handle_time();
    std::string handle_stats();
    std::string handle_shutdown();
    std::string handle_mirror(const std::string& message);
    bool is_command(const std::string& message);
};