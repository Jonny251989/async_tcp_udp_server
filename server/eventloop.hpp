#pragma once

#include <sys/epoll.h>
#include <functional>
#include <unordered_map>
#include <system_error>
#include <iostream>
#include <unistd.h>

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
    int epoll_fd_;
    bool running_;
    std::unordered_map<int, EventCallback> callbacks_;
    
    static constexpr int MAX_EVENTS = 64;
};