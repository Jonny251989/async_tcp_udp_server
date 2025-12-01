#include "command_processor.hpp"

CommandProcessor::CommandProcessor(std::vector<std::unique_ptr<Command>> &&commands) {
    for (auto &&command : commands) {
        command_map_[command->name()] = std::move(command);
    }
}

std::string CommandProcessor::process_command(const std::string& input) {
    std::string trimmed_input = input;
    trimmed_input.erase(trimmed_input.find_last_not_of(" \t\n\r\f\v") + 1);
    
    if (!is_command(trimmed_input)) {
        return handle_mirror(trimmed_input);
    }
    
    std::string command_name = trimmed_input.substr(1);
    auto it = command_map_.find(command_name);
    if (it != command_map_.end()) {
        return it->second->execute();
    }
    
    return "ERROR: Unknown command '" + trimmed_input + "'";
}

std::string CommandProcessor::handle_mirror(const std::string& message) {
    return message;
}

bool CommandProcessor::is_command(const std::string& message) {
    return !message.empty() && message[0] == '/';
}