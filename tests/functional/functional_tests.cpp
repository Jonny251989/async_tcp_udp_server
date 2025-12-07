#include <gtest/gtest.h>
#include <string>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

class FunctionalTest : public ::testing::Test {
protected:
    static constexpr int SERVER_PORT = 8080;
    static constexpr const char* SERVER_HOST = "127.0.0.1";
    
    bool wait_for_server(int timeout_seconds = 5) {
        for (int i = 0; i < timeout_seconds * 2; i++) {
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0) return false;
            
            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_port = htons(SERVER_PORT);
            inet_pton(AF_INET, SERVER_HOST, &addr.sin_addr);
            
            int flags = fcntl(sock, F_GETFL, 0);
            fcntl(sock, F_SETFL, flags | O_NONBLOCK);
            
            connect(sock, (struct sockaddr*)&addr, sizeof(addr));
            
            fd_set fdset;
            FD_ZERO(&fdset);
            FD_SET(sock, &fdset);
            struct timeval tv = {0, 100000}; // 100ms
            
            if (select(sock + 1, nullptr, &fdset, nullptr, &tv) == 1) {
                int so_error;
                socklen_t len = sizeof(so_error);
                getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);
                close(sock);
                if (so_error == 0) {
                    return true;
                }
            }
            
            close(sock);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        return false;
    }
    
    std::string send_tcp(const std::string& message) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) return "ERROR: socket failed";
        
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(SERVER_PORT);
        inet_pton(AF_INET, SERVER_HOST, &addr.sin_addr);
        
        struct timeval tv;
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        
        if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(sock);
            return "ERROR: connect failed";
        }
        
        std::string msg_with_newline = message + "\n";
        if (send(sock, msg_with_newline.c_str(), msg_with_newline.length(), 0) < 0) {
            close(sock);
            return "ERROR: send failed";
        }
        
        char buffer[1024] = {0};
        ssize_t bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        close(sock);
        
        if (bytes_received <= 0) {
            return "ERROR: receive failed";
        }
        
        buffer[bytes_received] = '\0';
        std::string result(buffer);
        
        size_t end = result.find_last_not_of("\r\n\t ");
        if (end != std::string::npos) {
            result = result.substr(0, end + 1);
        }
        
        return result;
    }
    
    std::string send_udp(const std::string& message) {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) return "ERROR: socket failed";
        
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(SERVER_PORT);
        inet_pton(AF_INET, SERVER_HOST, &addr.sin_addr);
        
        struct timeval tv;
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        
        std::string msg_with_newline = message + "\n";
        if (sendto(sock, msg_with_newline.c_str(), msg_with_newline.length(), 0,
                   (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(sock);
            return "ERROR: send failed";
        }
        
        char buffer[1024] = {0};
        socklen_t addr_len = sizeof(addr);
        ssize_t bytes_received = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                                          (struct sockaddr*)&addr, &addr_len);
        close(sock);
        
        if (bytes_received <= 0) {
            return "ERROR: receive failed";
        }
        
        buffer[bytes_received] = '\0';
        std::string result(buffer);
        
        size_t end = result.find_last_not_of("\r\n\t ");
        if (end != std::string::npos) {
            result = result.substr(0, end + 1);
        }
        
        return result;
    }
    
    void SetUp() override {
        if (!wait_for_server(3)) {
            std::cerr << "WARNING: Server not found on port " << SERVER_PORT 
                      << ". Make sure to run 'make functional-test' first." << std::endl;
        }
    }
};


TEST_F(FunctionalTest, TcpEcho) {
    std::string result = send_tcp("Hello");
    EXPECT_EQ(result, "Hello") << "Got: " << result;
}

TEST_F(FunctionalTest, TcpTime) {
    std::string result = send_tcp("/time");
    
    EXPECT_EQ(result.size(), 19) << "Got: " << result;
    EXPECT_EQ(result[4], '-') << "Got: " << result;
    EXPECT_EQ(result[7], '-') << "Got: " << result;
    EXPECT_EQ(result[10], ' ') << "Got: " << result;
    EXPECT_EQ(result[13], ':') << "Got: " << result;
    EXPECT_EQ(result[16], ':') << "Got: " << result;
}

TEST_F(FunctionalTest, TcpStats) {
    std::string result = send_tcp("/stats");
    
    EXPECT_TRUE(result.find("Total connections") != std::string::npos ||
                result.find("Current connections") != std::string::npos) 
        << "Got: " << result;
}

TEST_F(FunctionalTest, TcpShutdown) {
    std::string result = send_tcp("/shutdown");
    
    EXPECT_TRUE(result.find("shutdown") != std::string::npos ||
                result.find("Shutting down") != std::string::npos ||
                result == "/SHUTDOWN_ACK") 
        << "Got: " << result;
}

TEST_F(FunctionalTest, UdpEcho) {
    std::string result = send_udp("Hello UDP");
    EXPECT_EQ(result, "Hello UDP") << "Got: " << result;
}

TEST_F(FunctionalTest, UdpTime) {
    std::string result = send_udp("/time");
    
    EXPECT_EQ(result.size(), 19) << "Got: " << result;
    EXPECT_EQ(result[4], '-') << "Got: " << result;
    EXPECT_EQ(result[7], '-') << "Got: " << result;
    EXPECT_EQ(result[10], ' ') << "Got: " << result;
    EXPECT_EQ(result[13], ':') << "Got: " << result;
    EXPECT_EQ(result[16], ':') << "Got: " << result;
}
