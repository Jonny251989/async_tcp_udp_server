#include <gtest/gtest.h>
#include <fstream>
#include <cstdlib>
#include <string>
#include <thread>
#include <chrono>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <filesystem>
#include <cstdlib> // для getenv
#include <netdb.h> // для getaddrinfo
#include <cstring> // для memset

class FunctionalTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Получаем хост и порт из переменных окружения
        const char* env_host = std::getenv("SERVER_HOST");
        const char* env_port = std::getenv("SERVER_PORT");
        
        server_host_ = env_host ? env_host : "127.0.0.1";
        server_port_ = env_port ? std::atoi(env_port) : 8080;
        
        std::cout << "Testing against server: " << server_host_ << ":" << server_port_ << std::endl;
        
        // Ждем пока сервер станет доступен
        wait_for_server(server_host_, server_port_, 30);
    }

    void TearDown() override {
        // В Docker окружении не останавливаем сервер, так как он запущен отдельно
    }

    void wait_for_server(const std::string& host, uint16_t port, int max_attempts) {
        std::cout << "Waiting for server to start on " << host << ":" << port << "..." << std::endl;
        
        for (int i = 0; i < max_attempts; ++i) {
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }

            // Используем getaddrinfo для разрешения DNS имен (работает с Docker именами)
            struct addrinfo hints, *result;
            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            
            char port_str[6];
            snprintf(port_str, sizeof(port_str), "%d", port);
            
            int status = getaddrinfo(host.c_str(), port_str, &hints, &result);
            if (status != 0) {
                std::cerr << "getaddrinfo failed: " << gai_strerror(status) << std::endl;
                close(sock);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }

            if (connect(sock, result->ai_addr, result->ai_addrlen) == 0) {
                freeaddrinfo(result);
                close(sock);
                std::cout << "Server is ready after " << (i + 1) << " attempts" << std::endl;
                return;
            }
            
            freeaddrinfo(result);
            close(sock);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        throw std::runtime_error("Server not available after " + std::to_string(max_attempts) + " attempts");
    }

    std::string run_client_test(const std::string& test_name) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) == nullptr) {
            return "ERROR: getcwd() error";
        }
        
        std::string client_path;
        std::string message;
        
        if (test_name.find("udp") != std::string::npos) {
            client_path = std::string(cwd) + "/build/udp_client";
        } else {
            client_path = std::string(cwd) + "/build/tcp_client";
        }
        
        // Читаем сообщение из файла
        std::string input_file = "/test_data/" + test_name + "_input.txt";
        std::ifstream file(input_file);
        if (!file) {
            input_file = std::string(cwd) + "/tests/test_data/" + test_name + "_input.txt";
            file.open(input_file);
            if (!file) {
                return "ERROR: Cannot open input file: " + input_file;
            }
        }
        std::getline(file, message);
        
        // Запускаем клиент и захватываем вывод
        std::string command = client_path + " " + server_host_ + " " + 
                             std::to_string(server_port_) + " \"" + message + "\"";
        
        std::cout << "Running command: " << command << std::endl;
        
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            return "ERROR: Failed to run client";
        }
        
        char buffer[128];
        std::string result = "";
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        
        int status = pclose(pipe);
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            return "ERROR: Client execution failed with code: " + std::to_string(WEXITSTATUS(status));
        }
        
        // Удаляем завершающие пробелы и переводы строк
        if (!result.empty()) {
            result.erase(result.find_last_not_of(" \n\r\t") + 1);
        }
        return result;
    }

private:
    std::string server_host_ = "127.0.0.1";
    uint16_t server_port_ = 8080;
};

// Тесты остаются без изменений
TEST_F(FunctionalTest, EchoTcp) {
    const std::string output = run_client_test("echo_tcp");
    std::string response = output;
    if (output.find("Server response: ") == 0) {
        response = output.substr(17);
    }
    EXPECT_EQ(response, "Hello World") << "Client output: '" << output << "'";
}

TEST_F(FunctionalTest, TimeTcp) {
    const std::string output = run_client_test("time_tcp");
    std::string response = output;
    if (output.find("Server response: ") == 0) {
        response = output.substr(17);
    }
    EXPECT_EQ(response.size(), 19) << "Client output: '" << output << "'";
    EXPECT_EQ(response[4], '-') << "Client output: '" << output << "'";
    EXPECT_EQ(response[7], '-') << "Client output: '" << output << "'";
    EXPECT_EQ(response[10], ' ') << "Client output: '" << output << "'";
    EXPECT_EQ(response[13], ':') << "Client output: '" << output << "'";
    EXPECT_EQ(response[16], ':') << "Client output: '" << output << "'";
}

TEST_F(FunctionalTest, StatsTcp) {
    const std::string output = run_client_test("stats_tcp");
    EXPECT_NE(output.find("Total connections"), std::string::npos) 
        << "Client output: '" << output << "'";
    EXPECT_NE(output.find("Current connections"), std::string::npos)
        << "Client output: '" << output << "'";
}

TEST_F(FunctionalTest, EchoUdp) {
    const std::string output = run_client_test("echo_udp");
    std::string response = output;
    if (output.find("Server response: ") == 0) {
        response = output.substr(17);
    }
    EXPECT_EQ(response, "Hello UDP") << "Client output: '" << output << "'";
}

TEST_F(FunctionalTest, TimeUdp) {
    const std::string output = run_client_test("time_udp");
    std::string response = output;
    if (output.find("Server response: ") == 0) {
        response = output.substr(17);
    }
    EXPECT_EQ(response.size(), 19) << "Client output: '" << output << "'";
    EXPECT_EQ(response[4], '-') << "Client output: '" << output << "'";
    EXPECT_EQ(response[7], '-') << "Client output: '" << output << "'";
    EXPECT_EQ(response[10], ' ') << "Client output: '" << output << "'";
    EXPECT_EQ(response[13], ':') << "Client output: '" << output << "'";
    EXPECT_EQ(response[16], ':') << "Client output: '" << output << "'";
}