#include "command_processor.hpp"

CommandProcessor::CommandProcessor(std::vector<std::unique_ptr<Command>> &&commands) {

    for (auto &&command : commands) {
        std::string key = command->name();
        command_map_[key] = std::move(command);
    }
}

std::string CommandProcessor::process_command(const std::string& command) {
    if (!is_command(command)) {
        return handle_mirror(command);
    }
    
    auto it = command_map_.find(command);
    if (it != command_map_.end()) {
        return it->second->execute();
    }
    
    return "ERROR: Unknown command";
}

std::string CommandProcessor::handle_mirror(const std::string& message) {
    return message;
}

bool CommandProcessor::is_command(const std::string& message) {
    return !message.empty() && message[0] == '/';
}