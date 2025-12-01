#include <gtest/gtest.h>
#include <string>
#include <iostream>
#include <regex>

class FunctionalTest : public ::testing::Test {
protected:
    static constexpr int SERVER_PORT = 8080;
    static constexpr const char* SERVER_HOST = "127.0.0.1";
    
    std::string run_client(const std::string& protocol, const std::string& message) {
        // Простая команда - клиент должен быть в build/client_app
        std::string command = "echo \"" + message + "\" | timeout 2 ./build/client_app " + 
                             protocol + " " + SERVER_HOST + " " + std::to_string(SERVER_PORT);
        
        // Запускаем команду
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) return "ERROR: Failed to run client";
        
        // Читаем вывод
        char buffer[1024];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        
        pclose(pipe);
        
        // Убираем лишние пробелы
        result.erase(result.find_last_not_of(" \n\r\t") + 1);
        return result;
    }
};

// Простые тесты
TEST_F(FunctionalTest, EchoTcp) {
    EXPECT_EQ(run_client("tcp", "Hello"), "Hello");
}

TEST_F(FunctionalTest, EchoUdp) {
    EXPECT_EQ(run_client("udp", "Hello UDP"), "Hello UDP");
}

TEST_F(FunctionalTest, TimeTcp) {
    std::string result = run_client("tcp", "/time");
    EXPECT_EQ(result.size(), 19);
    EXPECT_EQ(result[4], '-');
    EXPECT_EQ(result[7], '-');
}

TEST_F(FunctionalTest, TimeUdp) {
    std::string result = run_client("udp", "/time");
    EXPECT_EQ(result.size(), 19);
    EXPECT_EQ(result[13], ':');
    EXPECT_EQ(result[16], ':');
}