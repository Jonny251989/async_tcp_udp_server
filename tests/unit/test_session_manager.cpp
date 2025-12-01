#include <gtest/gtest.h>
#include "../../server/session_manager.hpp"

TEST(SessionManagerTest, InitialState) {
    SessionManager manager;
    ServerStats stats = manager.get_stats();
    
    EXPECT_EQ(stats.total_connections, 0);
    EXPECT_EQ(stats.current_connections, 0);
}

TEST(SessionManagerTest, AddConnection) {
    SessionManager manager;
    
    manager.add_connection();
    ServerStats stats = manager.get_stats();
    
    EXPECT_EQ(stats.total_connections, 1);
    EXPECT_EQ(stats.current_connections, 1);
}

TEST(SessionManagerTest, RemoveConnection) {
    SessionManager manager;
    
    manager.add_connection();
    manager.add_connection();
    manager.remove_connection();
    
    ServerStats stats = manager.get_stats();
    EXPECT_EQ(stats.total_connections, 2);
    EXPECT_EQ(stats.current_connections, 1);
}

TEST(SessionManagerTest, IncrementTotalConnections) {
    SessionManager manager;
    
    manager.increment_total_connections();
    ServerStats stats = manager.get_stats();
    EXPECT_EQ(stats.total_connections, 1);
    EXPECT_EQ(stats.current_connections, 0);
}
