#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
SERVER="$BUILD_DIR/async_tcp_udp_server"
CLIENT="$BUILD_DIR/client_app"
SERVER_HOST="127.0.0.1"
SERVER_PORT=${1:-8080}

check_binaries() {
    if [ ! -f "$SERVER" ]; then
        echo "ERROR: Server not found at $SERVER"
        echo "Run 'make all' first"
        exit 1
    fi
    if [ ! -f "$CLIENT" ]; then
        echo "ERROR: Client not found at $CLIENT"
        echo "Run 'make all' first"
        exit 1
    fi
}

wait_for_server() {
    echo "Waiting for server to start..."
    for i in {1..10}; do
        if timeout 1 bash -c "cat < /dev/null > /dev/tcp/$SERVER_HOST/$SERVER_PORT" 2>/dev/null; then
            echo "✓ Server is ready!"
            return 0
        fi
        sleep 1
    done
    echo "ERROR: Server did not start in time"
    return 1
}

# Функция для отправки TCP сообщения через netcat
send_tcp() {
    local message="$1"
    echo -e "$message" | timeout 2 nc -N "$SERVER_HOST" "$SERVER_PORT" 2>/dev/null || true
}

# Функция для отправки UDP сообщения через netcat
send_udp() {
    local message="$1"
    echo -e "$message" | timeout 2 nc -u -w 1 "$SERVER_HOST" "$SERVER_PORT" 2>/dev/null || true
}

test_tcp_simple() {
    local failed_tests=0
    
    echo "=== TCP Tests ==="
    
    echo "Starting server..."
    $SERVER $SERVER_PORT > tcp_server.log 2>&1 &
    local server_pid=$!
    
    wait_for_server || {
        kill $server_pid 2>/dev/null || true
        return 1
    }
    
    echo "Test 1: Echo..."
    local echo_response=$(send_tcp "Hello")
    # Убираем лишние пробелы и переводы строк
    echo_response=$(echo "$echo_response" | xargs)
    
    if [ "$echo_response" == "Hello" ]; then
        echo "✓ Echo test passed"
    else
        echo "✗ Echo test failed: Got '$echo_response'"
        failed_tests=$((failed_tests + 1))
    fi
    
    echo "Test 2: Time..."
    local time_response=$(send_tcp "/time")
    time_response=$(echo "$time_response" | xargs)
    
    if [[ "$time_response" =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}\ [0-9]{2}:[0-9]{2}:[0-9]{2}$ ]]; then
        echo "✓ Time test passed"
    else
        echo "✗ Time test failed: Got '$time_response'"
        failed_tests=$((failed_tests + 1))
    fi
    
    echo "Test 3: Stats..."
    local stats_response=$(send_tcp "/stats")
    stats_response=$(echo "$stats_response" | xargs)
    
    if [[ "$stats_response" =~ Total\ connections:\ [0-9]+.*Current\ connections:\ [0-9]+ ]]; then
        echo "✓ Stats test passed: $stats_response"
    else
        echo "✗ Stats test failed: Got '$stats_response'"
        failed_tests=$((failed_tests + 1))
    fi
    
    echo "Test 4: Shutdown..."
    local shutdown_response=$(send_tcp "/shutdown")
    shutdown_response=$(echo "$shutdown_response" | xargs)
    
    if [[ "$shutdown_response" == "Server shutting down gracefully..." ]] || \
       [[ "$shutdown_response" == "/SHUTDOWN_ACK" ]] || \
       [[ "$shutdown_response" =~ shutdown ]]; then
        echo "✓ Shutdown test passed"
    else
        echo "✗ Shutdown test failed: Got '$shutdown_response'"
        failed_tests=$((failed_tests + 1))
    fi
    
    sleep 1
    kill $server_pid 2>/dev/null || true
    
    echo "TCP server log:"
    cat tcp_server.log
    rm -f tcp_server.log
    
    return $failed_tests
}

test_udp_simple() {
    local failed_tests=0
    
    echo ""
    echo "=== UDP Tests ==="
    
    echo "Starting server for UDP tests..."
    $SERVER $SERVER_PORT > udp_server.log 2>&1 &
    local server_pid=$!
    
    sleep 2
    
    echo "Test 1: UDP Echo..."
    local udp_echo_response=$(send_udp "Hello UDP")
    udp_echo_response=$(echo "$udp_echo_response" | xargs)
    
    if [ "$udp_echo_response" == "Hello UDP" ]; then
        echo "✓ UDP Echo test passed"
    else
        echo "✗ UDP Echo test failed: Got '$udp_echo_response'"
        failed_tests=$((failed_tests + 1))
    fi

    echo "Test 2: UDP Time..."
    local udp_time_response=$(send_udp "/time")
    udp_time_response=$(echo "$udp_time_response" | xargs)
    
    if [[ "$udp_time_response" =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}\ [0-9]{2}:[0-9]{2}:[0-9]{2}$ ]]; then
        echo "✓ UDP Time test passed"
    else
        echo "✗ UDP Time test failed: Got '$udp_time_response'"
        failed_tests=$((failed_tests + 1))
    fi
    
    kill $server_pid 2>/dev/null || true
    
    echo "UDP server log:"
    cat udp_server.log
    rm -f udp_server.log
    
    return $failed_tests
}

main() {
    echo "=== Functional Tests for Async TCP/UDP Server ==="
    echo "Using port: $SERVER_PORT"
    
    check_binaries
    
    local total_failed_tests=0
    
    test_tcp_simple
    tcp_failed=$?
    total_failed_tests=$((total_failed_tests + tcp_failed))
    
    test_udp_simple
    udp_failed=$?
    total_failed_tests=$((total_failed_tests + udp_failed))
    
    echo ""
    echo "=== Test Results ==="
    echo "TCP tests failed: $tcp_failed"
    echo "UDP tests failed: $udp_failed"
    echo "Total failed: $total_failed_tests"
    
    if [ $total_failed_tests -eq 0 ]; then
        echo "✓ All tests passed!"
        exit 0
    else
        echo "✗ $total_failed_tests test(s) failed"
        exit 1
    fi
}

main "$@"