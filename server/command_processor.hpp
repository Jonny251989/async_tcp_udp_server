#pragma once

#include "command.hpp"
#include <vector>
#include <memory>
#include <unordered_map>
#include <string>

class CommandProcessor {
public:
    explicit CommandProcessor(std::vector<std::unique_ptr<Command>> commands);
    
    std::string process_command(const std::string& command);

private:
    std::string handle_mirror(const std::string& message);
    bool is_command(const std::string& message);

    std::vector<std::unique_ptr<Command>> commands_;
    std::unordered_map<std::string, Command*> command_map_;
};