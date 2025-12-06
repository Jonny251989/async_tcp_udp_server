#ifndef SERVER_HPP
#define SERVER_HPP

#include <memory>
#include <atomic>
#include <vector>
#include <functional>
#include "signal_handler.hpp"
#include "tcp_handler.hpp"
#include "udp_handler.hpp"
#include "command_processor.hpp"
#include "session_manager.hpp"
#include "eventloop.hpp"

class Server : public std::enable_shared_from_this<Server> {
public:
    Server(uint16_t port);
    ~Server();
    
    bool start();
    void stop();
    void run();
    
    // For signal handler
    void request_shutdown();
    
private:
    std::vector<std::unique_ptr<Command>> create_commands();
    void setup_signal_handler();
    void setup_tcp_handler();
    void setup_udp_handler();
    
    void handle_tcp_connection(std::shared_ptr<TcpConnection> connection);
    void handle_tcp_message(const std::string& message, std::shared_ptr<TcpConnection> connection);
    void handle_udp_message(const std::string& message, const sockaddr_in& client_addr);
    
    uint16_t port_;
    std::shared_ptr<SessionManager> session_manager_;
    CommandProcessor command_processor_;
    std::atomic<bool> shutdown_requested_;
    
    std::unique_ptr<TcpHandler> tcp_handler_;
    std::unique_ptr<UdpHandler> udp_handler_;
    EventLoop event_loop_;
};

#endif // SERVER_HPP