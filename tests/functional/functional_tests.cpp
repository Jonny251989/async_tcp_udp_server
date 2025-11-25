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

class FunctionalTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Получаем абсолютный путь к серверу
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) == nullptr) {
            FAIL() << "getcwd() error";
        }
        std::string server_path = std::string(cwd) + "/build/telemetry_server";
        
        std::cout << "Starting server from: " << server_path << std::endl;
        
        // Пробуем разные порты если 8080 занят
        for (int port = 8080; port <= 8090; ++port) {
            if (start_server(port)) {
                server_port_ = port;
                std::cout << "Server started on port " << port << std::endl;
                return;
            }
        }
        FAIL() << "Could not start server on any port between 8080-8090";
    }

    void TearDown() override {
        // Останавливаем сервер
        if (server_pid > 0) {
            kill(server_pid, SIGTERM);
            waitpid(server_pid, nullptr, 0);
            std::cout << "Server stopped" << std::endl;
        }
    }

    bool start_server(uint16_t port) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) == nullptr) {
            return false;
        }
        std::string server_path = std::string(cwd) + "/build/telemetry_server";
        
        // Запускаем сервер в фоне
        server_pid = fork();
        if (server_pid == 0) {
            // Дочерний процесс - запускаем сервер
            if (chdir(cwd) != 0) {
                perror("chdir");
                exit(1);
            }
            execl(server_path.c_str(), "telemetry_server", std::to_string(port).c_str(), nullptr);
            perror("execl failed");
            exit(1);
        }
        
        // Ждем, пока сервер запустится
        return wait_for_server(port, 10);
    }

    bool wait_for_server(uint16_t port, int max_attempts) {
        std::cout << "Waiting for server to start on port " << port << "..." << std::endl;
        for (int i = 0; i < max_attempts; ++i) {
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }

            sockaddr_in serv_addr{};
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(port);
            serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

            if (connect(sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == 0) {
                close(sock);
                std::cout << "Server is ready after " << (i + 1) << " attempts" << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                return true;
            }
            close(sock);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        std::cout << "Server not available after " << max_attempts << " attempts" << std::endl;
        return false;
    }

    std::string run_client_test(const std::string& test_name) {
        // Используем прямые вызовы клиентов вместо скрипта
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
        std::string input_file = std::string(cwd) + "/tests/test_data/" + test_name + "_input.txt";
        std::ifstream file(input_file);
        if (!file) {
            return "ERROR: Cannot open input file: " + input_file;
        }
        std::getline(file, message);
        
        // Запускаем клиент и захватываем вывод
        std::string command = client_path + " 127.0.0.1 " + std::to_string(server_port_) + " \"" + message + "\"";
        
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
    pid_t server_pid = -1;
    uint16_t server_port_ = 8080;
};

TEST_F(FunctionalTest, EchoTcp) {
    const std::string output = run_client_test("echo_tcp");
    // Извлекаем только часть ответа после "Server response: "
    std::string response = output;
    if (output.find("Server response: ") == 0) {
        response = output.substr(17); // Убираем "Server response: "
    }
    EXPECT_EQ(response, "Hello World") << "Client output: '" << output << "'";
}

TEST_F(FunctionalTest, TimeTcp) {
    const std::string output = run_client_test("time_tcp");
    std::string response = output;
    if (output.find("Server response: ") == 0) {
        response = output.substr(17);
    }
    
    // Проверяем формат времени: YYYY-MM-DD HH:MM:SS
    if (response.find("ERROR") == std::string::npos) {
        EXPECT_EQ(response.size(), 19) << "Client output: '" << output << "'";
        EXPECT_EQ(response[4], '-') << "Client output: '" << output << "'";
        EXPECT_EQ(response[7], '-') << "Client output: '" << output << "'";
        EXPECT_EQ(response[10], ' ') << "Client output: '" << output << "'";
        EXPECT_EQ(response[13], ':') << "Client output: '" << output << "'";
        EXPECT_EQ(response[16], ':') << "Client output: '" << output << "'";
    } else {
        FAIL() << "Command failed: " << output;
    }
}

TEST_F(FunctionalTest, StatsTcp) {
    const std::string output = run_client_test("stats_tcp");
    // Статистика должна содержать информацию о подключениях
    if (output.find("ERROR") == std::string::npos) {
        EXPECT_NE(output.find("Total connections"), std::string::npos) 
            << "Client output: '" << output << "'";
        EXPECT_NE(output.find("Current connections"), std::string::npos)
            << "Client output: '" << output << "'";
    } else {
        FAIL() << "Command failed: " << output;
    }
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
    
    // Проверяем формат времени
    if (response.find("ERROR") == std::string::npos) {
        EXPECT_EQ(response.size(), 19) << "Client output: '" << output << "'";
        EXPECT_EQ(response[4], '-') << "Client output: '" << output << "'";
        EXPECT_EQ(response[7], '-') << "Client output: '" << output << "'";
        EXPECT_EQ(response[10], ' ') << "Client output: '" << output << "'";
        EXPECT_EQ(response[13], ':') << "Client output: '" << output << "'";
        EXPECT_EQ(response[16], ':') << "Client output: '" << output << "'";
    } else {
        FAIL() << "Command failed: " << output;
    }
}