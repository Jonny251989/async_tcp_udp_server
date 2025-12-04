#include "eventloop.hpp"

EventLoop::EventLoop() : epoll_fd_(-1) {
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
        return false;
    }
    
    callbacks_[fd] = std::move(callback);
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
    return true;
}

void EventLoop::run(int timeout_ms) {
    epoll_event events[MAX_EVENTS];
    
    int num_events = epoll_wait(epoll_fd_, events, MAX_EVENTS, timeout_ms);
    
    if (num_events == -1) {
        if (errno == EINTR) {
            return;
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