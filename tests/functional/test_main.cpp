#include <gtest/gtest.h>
#include <iostream>
#include <cstdlib>

int main(int argc, char **argv) {
    std::cout << "=========================================" << std::endl;
    std::cout << "Running Functional Tests for Async TCP/UDP Server" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "Requirements:" << std::endl;
    std::cout << "1. Server will be started on 127.0.0.1:8080" << std::endl;
    std::cout << "2. Client binary should be in build/client" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    setenv("SERVER_HOST", "127.0.0.1", 1);
    setenv("SERVER_PORT", "8080", 1);
    
    ::testing::InitGoogleTest(&argc, argv);
    
    int result = RUN_ALL_TESTS();
    
    std::cout << "=========================================" << std::endl;
    if (result == 0) {
        std::cout << "✓ All Functional Tests PASSED!" << std::endl;
    } else {
        std::cout << "✗ Some Functional Tests FAILED!" << std::endl;
    }
    std::cout << "=========================================" << std::endl;
    
    return result;
}