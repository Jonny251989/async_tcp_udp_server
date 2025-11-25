#include <gtest/gtest.h>
#include <iostream>

int main(int argc, char **argv) {
    std::cout << "=========================================" << std::endl;
    std::cout << "Running Functional Tests for Async TCP/UDP Server" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "Note: This will start the server in background" << std::endl;
    std::cout << "and run client tests against it." << std::endl;
    std::cout << "=========================================" << std::endl;
    
    ::testing::InitGoogleTest(&argc, argv);
    
    int result = RUN_ALL_TESTS();
    
    std::cout << "=========================================" << std::endl;
    if (result == 0) {
        std::cout << "All Functional Tests PASSED!" << std::endl;
    } else {
        std::cout << "Some Functional Tests FAILED!" << std::endl;
    }
    std::cout << "=========================================" << std::endl;
    
    return result;
}