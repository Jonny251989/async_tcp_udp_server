#ifndef SIGNAL_HANDLER_HPP
#define SIGNAL_HANDLER_HPP

#include <poll.h>
#include <signal.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <cstring>

#include <atomic>
#include <csignal>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <system_error>
#include <thread>
#include <vector>
#include <tuple>
#include <utility>

class SignalHandler {
public:

    static SignalHandler& instance() {
        static SignalHandler instance;
        return instance;
    }

    SignalHandler(const SignalHandler&) = delete;
    SignalHandler& operator=(const SignalHandler&) = delete;
    SignalHandler(SignalHandler&&) = delete;
    SignalHandler& operator=(SignalHandler&&) = delete;

    template <typename CallbackT, typename... Args>
    void set_handler(const std::vector<int>& signals, CallbackT&& callback, Args&&... args) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (running_) {
            throw std::runtime_error("SignalHandler is already running. Call reset() first.");
        }

        if ((terminate_event_fd_ = eventfd(0, EFD_CLOEXEC)) < 0) {
            throw std::system_error(errno, std::system_category(),
                                    "unable to create terminate event FD");
        }
        if ((trigger_event_fd_ = eventfd(0, EFD_CLOEXEC | EFD_SEMAPHORE)) < 0) {
            close(terminate_event_fd_);
            throw std::system_error(errno, std::system_category(),
                                    "unable to create trigger event FD");
        }

        signals_ = signals;

        thread_ = std::thread([this, callback = std::forward<CallbackT>(callback),
                              args_tuple = std::make_tuple(std::forward<Args>(args)...)]() mutable {
            this->trigger_handler_impl(std::move(callback), std::move(args_tuple));
        });

        install_signals();
        running_ = true;
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!running_) {
            return;
        }

        uninstall_signals();
        
        if (terminate_event_fd_ != -1) {
            eventfd_write(terminate_event_fd_, 1);
        }

        if (thread_.joinable()) {
            thread_.join();
        }

        if (terminate_event_fd_ != -1) {
            close(terminate_event_fd_);
            terminate_event_fd_ = -1;
        }
        if (trigger_event_fd_ != -1) {
            close(trigger_event_fd_);
            trigger_event_fd_ = -1;
        }
        
        running_ = false;
        thread_ready_.store(false, std::memory_order_release);
    }

    bool is_running() const {
        return running_;
    }

private:
    SignalHandler() = default;

    ~SignalHandler() {
        reset();
    }

    void install_signals() {
        struct sigaction act;
        act.sa_sigaction = trigger_emitter;
        sigemptyset(&act.sa_mask);
        act.sa_flags = SA_SIGINFO | SA_RESTART;
        
        for (int sig : signals_) {
            if (sigaction(sig, &act, nullptr) == -1) {
                throw std::system_error(errno, std::system_category(), 
                                        "sigaction install failed for signal " + std::to_string(sig));
            }
        }
    }

    void uninstall_signals() {
        struct sigaction act;
        act.sa_handler = SIG_DFL;
        sigemptyset(&act.sa_mask);
        act.sa_flags = 0;
        
        for (int sig : signals_) {
            sigaction(sig, &act, nullptr); 
        }
    }

    static void trigger_emitter(int signum, siginfo_t* info, void* context) {
        SignalHandler::instance().trigger();
    }

    void trigger() {
        if (trigger_event_fd_ != -1) {
            eventfd_write(trigger_event_fd_, 1);
        }
    }

    template <typename CallbackT, typename ArgsTuple, size_t... I>
    void trigger_handler_impl(CallbackT&& callback, ArgsTuple&& args, std::index_sequence<I...>) {
        uint64_t val;
        struct pollfd pfds[2] = {
            {.fd = terminate_event_fd_, .events = POLLIN, .revents = 0},
            {.fd = trigger_event_fd_, .events = POLLIN, .revents = 0}
        };

        thread_ready_.store(true, std::memory_order_release);

        while (true) {
            int ret = poll(pfds, 2, -1);
            if (ret < 0) {
                if (errno == EINTR) {
                    continue;
                }
                std::cerr << "Failed on poll: " << strerror(errno) << std::endl;
                break;
            }

            if (pfds[0].revents & POLLIN) {
                auto count = read(terminate_event_fd_, &val, sizeof(val));
                if (count < 0) {
                    std::cerr << "Failed to read termination event: " << strerror(errno) << std::endl;
                }
                break;
            }

            if (pfds[1].revents & POLLIN) {
                auto count = read(trigger_event_fd_, &val, sizeof(val));
                if (count < 0) {
                    std::cerr << "Failed to read trigger event: " << strerror(errno) << std::endl;
                    break;
                }
                
                try {
                    std::forward<CallbackT>(callback)(std::get<I>(std::forward<ArgsTuple>(args))...);
                } catch (const std::exception& e) {
                    std::cerr << "Exception in signal callback: " << e.what() << std::endl;
                }
            }
        }
    }

    template <typename CallbackT, typename ArgsTuple>
    void trigger_handler_impl(CallbackT&& callback, ArgsTuple&& args) {
        using Indices = std::make_index_sequence<std::tuple_size_v<std::decay_t<ArgsTuple>>>;
        trigger_handler_impl(std::forward<CallbackT>(callback), std::forward<ArgsTuple>(args),
                            Indices{});
    }

private:
    std::vector<int> signals_;
    int terminate_event_fd_ = -1;
    int trigger_event_fd_ = -1;
    std::thread thread_;
    std::mutex mutex_;
    std::atomic<bool> thread_ready_{false};
    std::atomic<bool> running_{false};
};

template <typename CallbackT, typename... Args>
void set_signal_handler(const std::vector<int>& signals, CallbackT&& callback, Args&&... args) {
    SignalHandler::instance().set_handler(signals, std::forward<CallbackT>(callback),
                                         std::forward<Args>(args)...);
}

inline void reset_signal_handler() {
    SignalHandler::instance().reset();
}

#endif