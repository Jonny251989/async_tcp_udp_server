#include <gtest/gtest.h>
#include <iostream>

int main(int argc, char **argv) {
    std::cout << "=========================================" << std::endl;
    std::cout << "Running Unit Tests for Async TCP/UDP Server" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    ::testing::InitGoogleTest(&argc, argv);
    
    int result = RUN_ALL_TESTS();
    
    std::cout << "=========================================" << std::endl;
    std::cout << "Unit Tests Completed" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    return result;
}