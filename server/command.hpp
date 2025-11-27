#pragma once

#include <string>
#include <memory>
#include "../common/session_manager.hpp"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <ctime>

// Абстрактный базовый класс для команд
class Command {
public:
    virtual ~Command() = default;
    virtual std::string name() const = 0;
    virtual std::string execute() = 0;
};

// Конкретные реализации команд
class TimeCommand : public Command {
public:
    std::string name() const override { return "/time"; }
    std::string execute() override;
};

class StatsCommand : public Command {
public:
    explicit StatsCommand(SessionManager& session_manager);
    std::string name() const override { return "/stats"; }
    std::string execute() override;

private:
    SessionManager& session_manager_;
};

class ShutdownCommand : public Command {
public:
    std::string name() const override { return "/shutdown"; }
    std::string execute() override;
};