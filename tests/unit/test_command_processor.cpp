#include <gtest/gtest.h>
#include <string>
#include <memory>
#include <vector>

#include "../../server/command_processor.hpp"
#include "../../server/session_manager.hpp"
#include "../../server/command.hpp"

class CommandProcessorTest : public ::testing::Test {
protected:
    void SetUp() override {
        session_manager = std::make_shared<SessionManager>();
        
        std::vector<std::unique_ptr<Command>> commands;
        commands.push_back(std::make_unique<TimeCommand>());
        commands.push_back(std::make_unique<StatsCommand>(*session_manager));
        commands.push_back(std::make_unique<ShutdownCommand>());
        
        processor = std::make_unique<CommandProcessor>(std::move(commands));
    }
    
    std::shared_ptr<SessionManager> session_manager;
    std::unique_ptr<CommandProcessor> processor;
};

TEST_F(CommandProcessorTest, ProcessTimeCommand) {
    std::string result = processor->process_command("/time");
    // Проверяем формат времени: YYYY-MM-DD HH:MM:SS
    EXPECT_EQ(result.size(), 19);
    EXPECT_EQ(result[4], '-');
    EXPECT_EQ(result[7], '-');
    EXPECT_EQ(result[10], ' ');
    EXPECT_EQ(result[13], ':');
    EXPECT_EQ(result[16], ':');
}

TEST_F(CommandProcessorTest, ProcessStatsCommand) {
    // Добавляем тестовое соединение (без параметров)
    session_manager->add_connection();
    
    std::string result = processor->process_command("/stats");
    EXPECT_NE(result.find("Total connections"), std::string::npos);
    EXPECT_NE(result.find("Current connections"), std::string::npos);
    EXPECT_NE(result.find("1"), std::string::npos); // Должна быть хотя бы 1 связь
    
    session_manager->remove_connection();
}

TEST_F(CommandProcessorTest, ProcessShutdownCommand) {
    std::string result = processor->process_command("/shutdown");
    EXPECT_EQ(result, "/SHUTDOWN_ACK");
}

TEST_F(CommandProcessorTest, ProcessUnknownCommand) {
    std::string result = processor->process_command("/unknown");
    // Убедитесь что сообщение об ошибке правильное
    // Если CommandProcessor возвращает "ERROR: Unknown command '/unknown'", то:
    EXPECT_EQ(result, "ERROR: Unknown command '/unknown'");
    // Или хотя бы проверьте что начинается с "ERROR: Unknown command"
    // EXPECT_TRUE(result.find("ERROR: Unknown command") != std::string::npos);
}

TEST_F(CommandProcessorTest, ProcessEchoMessage) {
    std::string result = processor->process_command("Hello World");
    EXPECT_EQ(result, "Hello World");
}

TEST_F(CommandProcessorTest, ProcessEmptyMessage) {
    std::string result = processor->process_command("");
    EXPECT_EQ(result, "");
}