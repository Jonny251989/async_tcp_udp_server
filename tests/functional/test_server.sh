#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
SERVER="$BUILD_DIR/async_tcp_udp_server"
CLIENT="$BUILD_DIR/client_app"
SERVER_HOST="127.0.0.1"
SERVER_PORT=8080

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
    local output1=$(echo "Hello" | timeout 2 "$CLIENT" "tcp" "$SERVER_HOST" "$SERVER_PORT" 2>&1)
    local echo_response=$(echo "$output1" | grep -v "Connected to TCP" | head -n 1 | xargs)
    
    if [ "$echo_response" == "Hello" ]; then
        echo "✓ Echo test passed"
    else
        echo "✗ Echo test failed: Got '$echo_response'"
        failed_tests=$((failed_tests + 1))
    fi
    
    echo "Test 2: Time..."
    local output2=$(echo "/time" | timeout 2 "$CLIENT" "tcp" "$SERVER_HOST" "$SERVER_PORT" 2>&1)
    local time_response=$(echo "$output2" | grep -v "Connected to TCP" | head -n 1 | xargs)
    
    if [[ "$time_response" =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}\ [0-9]{2}:[0-9]{2}:[0-9]{2}$ ]]; then
        echo "✓ Time test passed"
    else
        echo "✗ Time test failed: Got '$time_response'"
        failed_tests=$((failed_tests + 1))
    fi
    
    echo "Test 3: Stats..."
    local output3=$(echo "/stats" | timeout 2 "$CLIENT" "tcp" "$SERVER_HOST" "$SERVER_PORT" 2>&1)
    local stats_response=$(echo "$output3" | grep -v "Connected to TCP" | xargs)
    
    if [[ "$stats_response" =~ Total\ connections:\ [0-9]+.*Current\ connections:\ [0-9]+ ]]; then
        echo "✓ Stats test passed: $stats_response"
    else
        echo "✗ Stats test failed: Got '$stats_response'"
        failed_tests=$((failed_tests + 1))
    fi
    
    echo "Test 4: Shutdown..."
    local output4=$(echo "/shutdown" | timeout 2 "$CLIENT" "tcp" "$SERVER_HOST" "$SERVER_PORT" 2>&1)
    local shutdown_response=$(echo "$output4" | grep -v "Connected to TCP" | head -n 1 | xargs)
    
    if [ "$shutdown_response" == "Server shutting down gracefully..." ]; then
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
    local udp_output1=$(echo "Hello UDP" | timeout 2 "$CLIENT" "udp" "$SERVER_HOST" "$SERVER_PORT" 2>&1)
    local udp_echo_response=$(echo "$udp_output1" | grep -v "UDP client ready" | head -n 1 | xargs)
    
    if [ "$udp_echo_response" == "Hello UDP" ]; then
        echo "✓ UDP Echo test passed"
    else
        echo "✗ UDP Echo test failed: Got '$udp_echo_response'"
        failed_tests=$((failed_tests + 1))
    fi

    echo "Test 2: UDP Time..."
    local udp_output2=$(echo "/time" | timeout 2 "$CLIENT" "udp" "$SERVER_HOST" "$SERVER_PORT" 2>&1)
    local udp_time_response=$(echo "$udp_output2" | grep -v "UDP client ready" | head -n 1 | xargs)
    
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