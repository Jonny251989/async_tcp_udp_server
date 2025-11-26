#include "command_processor.hpp"
#include <memory>

CommandProcessor::CommandProcessor(std::vector<std::unique_ptr<Command>> commands) 
    : commands_(std::move(commands)) {
    
    // Инициализируем карту команд для быстрого доступа
    for (auto& command : commands_) {
        command_map_[command->name()] = command.get();
    }
}

std::string CommandProcessor::process_command(const std::string& command) {
    // Проверяем, является ли сообщение командой
    if (!is_command(command)) {
        return handle_mirror(command);
    }
    
    // Ищем команду в карте
    auto it = command_map_.find(command);
    if (it != command_map_.end()) {
        return it->second->execute();
    }
    
    return "ERROR: Unknown command";
}

std::string CommandProcessor::handle_mirror(const std::string& message) {
    // Просто возвращаем полученное сообщение обратно
    return message;
}

bool CommandProcessor::is_command(const std::string& message) {
    return !message.empty() && message[0] == '/';
}