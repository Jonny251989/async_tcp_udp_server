#pragma once

#include <functional>
#include <unordered_map>
#include <atomic>
#include <unistd.h>
#include <sys/epoll.h>
#include <system_error>
#include <cstring>

#include <sys/epoll.h>
#include <fcntl.h>

#include <iostream>

class EventLoop {
public:
    using EventCallback = std::function<void(uint32_t)>;
    
    EventLoop();
    ~EventLoop();
    
    bool add_fd(int fd, uint32_t events, EventCallback callback);
    bool modify_fd(int fd, uint32_t events);
    bool remove_fd(int fd);
    
    void run(int timeout_ms);
    void stop();
    void stop_immediate();  // Новый метод для немедленной остановки

private:
    static const int MAX_EVENTS = 64;
    
    int epoll_fd_;
    int wakeup_fd_[2];  // Pipe для пробуждения
    std::unordered_map<int, EventCallback> callbacks_;
    std::atomic<bool> running_{false};
};