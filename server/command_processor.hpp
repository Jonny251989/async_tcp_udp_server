#pragma once

#include "command.hpp"
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>

class CommandProcessor {
public:
    CommandProcessor(std::vector<std::unique_ptr<Command>> &&commands);
    std::string process_command(const std::string& input);

private:
    std::string handle_mirror(const std::string& message);
    bool is_command(const std::string& message);
    
    std::unordered_map<std::string, std::unique_ptr<Command>> command_map_;
};