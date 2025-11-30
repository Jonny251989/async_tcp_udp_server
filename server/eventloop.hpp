#pragma once

#include <functional>
#include <unordered_map>
#include <atomic>

class EventLoop {
public:
    using EventCallback = std::function<void(uint32_t events)>;
    
    EventLoop();
    ~EventLoop();
    
    bool add_fd(int fd, uint32_t events, EventCallback callback);
    bool modify_fd(int fd, uint32_t events);
    bool remove_fd(int fd);
    
    void run(int timeout_ms = -1);
    void stop();

private:
    static const int MAX_EVENTS = 64;
    
    int epoll_fd_;
    std::atomic<bool> running_;
    std::unordered_map<int, EventCallback> callbacks_;
};