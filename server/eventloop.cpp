#include "eventloop.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/epoll.h>
#include <system_error>
#include <cstring>

EventLoop::EventLoop() 
    : epoll_fd_(-1)
    , running_(false) {
    
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ == -1) {
        throw std::system_error(errno, std::system_category(), "epoll_create1 failed");
    }
}

EventLoop::~EventLoop() {
    if (epoll_fd_ != -1) {
        close(epoll_fd_);
    }
}

bool EventLoop::add_fd(int fd, uint32_t events, EventCallback callback) {
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
        std::cerr << "Failed to add fd " << fd << " to epoll: " << strerror(errno) << std::endl;
        return false;
    }
    
    callbacks_[fd] = std::move(callback);
    std::cout << "✓ Added fd " << fd << " to event loop" << std::endl;
    return true;
}

bool EventLoop::modify_fd(int fd, uint32_t events) {
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    return epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev) != -1;
}

bool EventLoop::remove_fd(int fd) {
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr) == -1) {
        return false;
    }
    callbacks_.erase(fd);
    std::cout << "✓ Removed fd " << fd << " from event loop" << std::endl;
    return true;
}

void EventLoop::stop() {
    running_ = false;
}

void EventLoop::run(int timeout_ms) {
    if (!running_) {
        running_ = true;
    }
    
    epoll_event events[MAX_EVENTS];
    
    int num_events = epoll_wait(epoll_fd_, events, MAX_EVENTS, timeout_ms);
    
    if (num_events == -1) {
        if (errno != EINTR) {
            std::cerr << "epoll_wait error: " << strerror(errno) << std::endl;
        }
        return;
    }
    
    for (int i = 0; i < num_events; ++i) {
        int fd = events[i].data.fd;
        uint32_t ev_events = events[i].events;
        
        auto it = callbacks_.find(fd);
        if (it != callbacks_.end()) {
            it->second(ev_events);
        }
    }
}