#include "command_processor.hpp"
#include <iostream>

CommandProcessor::CommandProcessor(std::vector<std::unique_ptr<Command>> &&commands) {
    std::cout << "=== COMMANDPROCESSOR INIT ===" << std::endl;
    std::cout << "Received " << commands.size() << " commands" << std::endl;
    
    for (auto &&command : commands) {
        std::string key = command->name();
        std::cout << "Registering command: '" << key << "'" << std::endl;
        command_map_[key] = std::move(command);
    }
    
    std::cout << "Total commands in map: " << command_map_.size() << std::endl;
    std::cout << "=== COMMANDPROCESSOR READY ===" << std::endl;
}

std::string CommandProcessor::process_command(const std::string& input) {
    std::string trimmed_input = input;
    trimmed_input.erase(trimmed_input.find_last_not_of(" \t\n\r\f\v") + 1);
    
    std::cout << "CommandProcessor: processing '" << trimmed_input << "'" << std::endl;
    
    if (!is_command(trimmed_input)) {
        std::cout << "CommandProcessor: mirroring non-command message" << std::endl;
        return handle_mirror(trimmed_input);
    }
    
    // Убираем слеш для поиска в command_map_
    std::string command_name = trimmed_input.substr(1);  // ← УБИРАЕМ ПЕРВЫЙ СИМВОЛ (слеш)
    std::cout << "CommandProcessor: looking for command: '" << command_name << "'" << std::endl;
    std::cout << "Available commands in map: ";
    for (const auto& pair : command_map_) {
        std::cout << "'" << pair.first << "' ";
    }
    std::cout << std::endl;
    
    auto it = command_map_.find(command_name);  // ← Ищем БЕЗ слеша
    if (it != command_map_.end()) {
        std::cout << "CommandProcessor: FOUND command '" << command_name << "'" << std::endl;
        return it->second->execute();
    }
    
    std::cout << "CommandProcessor: UNKNOWN command '" << command_name << "'" << std::endl;
    return "ERROR: Unknown command '" + trimmed_input + "'";
}

std::string CommandProcessor::handle_mirror(const std::string& message) {
    return message;
}

bool CommandProcessor::is_command(const std::string& message) {
    return !message.empty() && message[0] == '/';
}