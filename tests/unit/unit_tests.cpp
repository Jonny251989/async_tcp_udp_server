#include <gtest/gtest.h>
#include <string>
#include <sstream>

// Включаем заголовочные файлы проекта
#include "../../server/command_processor.hpp"
#include "../../server/session_manager.hpp"
#include "../../common/protocol.hpp"

// Тесты для CommandProcessor
class CommandProcessorTest : public ::testing::Test {
protected:
    SessionManager session_manager;
    CommandProcessor command_processor{session_manager};
};

TEST_F(CommandProcessorTest, EchoNonCommand) {
    std::string result = command_processor.process_command("hello");
    EXPECT_EQ(result, "hello");
}

TEST_F(CommandProcessorTest, TimeCommand) {
    std::string result = command_processor.process_command("/time");
    // Проверяем формат времени: YYYY-MM-DD HH:MM:SS
    EXPECT_EQ(result.size(), 19);
    EXPECT_EQ(result[4], '-');
    EXPECT_EQ(result[7], '-');
    EXPECT_EQ(result[10], ' ');
    EXPECT_EQ(result[13], ':');
    EXPECT_EQ(result[16], ':');
}

TEST_F(CommandProcessorTest, StatsCommand) {
    // Добавляем соединение для теста статистики
    session_manager.add_connection(1);
    
    std::string result = command_processor.process_command("/stats");
    EXPECT_NE(result.find("Total connections"), std::string::npos);
    EXPECT_NE(result.find("Current connections"), std::string::npos);
    
    session_manager.remove_connection(1);
}

TEST_F(CommandProcessorTest, ShutdownCommand) {
    std::string result = command_processor.process_command("/shutdown");
    EXPECT_EQ(result, "/SHUTDOWN_ACK");
}

TEST_F(CommandProcessorTest, UnknownCommand) {
    std::string result = command_processor.process_command("/unknown");
    EXPECT_EQ(result, "ERROR: Unknown command");
}

TEST_F(CommandProcessorTest, EmptyMessage) {
    std::string result = command_processor.process_command("");
    EXPECT_EQ(result, "");
}

// Тесты для SessionManager
class SessionManagerTest : public ::testing::Test {
protected:
    SessionManager session_manager;
};

TEST_F(SessionManagerTest, InitialStats) {
    ServerStats stats = session_manager.get_stats();
    EXPECT_EQ(stats.total_connections, 0);
    EXPECT_EQ(stats.current_connections, 0);
}

TEST_F(SessionManagerTest, AddConnection) {
    session_manager.add_connection(1);
    ServerStats stats = session_manager.get_stats();
    EXPECT_EQ(stats.total_connections, 1);
    EXPECT_EQ(stats.current_connections, 1);
    
    session_manager.remove_connection(1);
}

TEST_F(SessionManagerTest, RemoveConnection) {
    session_manager.add_connection(1);
    session_manager.add_connection(2);
    
    session_manager.remove_connection(1);
    ServerStats stats = session_manager.get_stats();
    EXPECT_EQ(stats.total_connections, 2);  // total не уменьшается
    EXPECT_EQ(stats.current_connections, 1); // current уменьшается
    
    session_manager.remove_connection(2);
}

TEST_F(SessionManagerTest, IncrementTotalConnections) {
    session_manager.increment_total_connections();
    ServerStats stats = session_manager.get_stats();
    EXPECT_EQ(stats.total_connections, 1);
    EXPECT_EQ(stats.current_connections, 0);
}

TEST_F(SessionManagerTest, MultipleConnections) {
    session_manager.add_connection(1);
    session_manager.add_connection(2);
    session_manager.add_connection(3);
    
    ServerStats stats = session_manager.get_stats();
    EXPECT_EQ(stats.total_connections, 3);
    EXPECT_EQ(stats.current_connections, 3);
    
    session_manager.remove_connection(1);
    session_manager.remove_connection(2);
    session_manager.remove_connection(3);
}

// Тесты для protocol функций
TEST(ProtocolTest, GetCurrentTimeString) {
    std::string time_str = get_current_time_string();
    EXPECT_EQ(time_str.size(), 19);
    EXPECT_EQ(time_str[4], '-');
    EXPECT_EQ(time_str[7], '-');
    EXPECT_EQ(time_str[10], ' ');
    EXPECT_EQ(time_str[13], ':');
    EXPECT_EQ(time_str[16], ':');
}