#!/bin/bash

set -e

echo "=== Systemd Integration Test ==="

if [ "$EUID" -ne 0 ]; then 
    echo "Please run as root (sudo)"
    exit 1
fi

echo "1. Stopping existing service..."
systemctl stop async-tcp-udp-server 2>/dev/null || true

echo "2. Starting service..."
systemctl start async-tcp-udp-server

echo "3. Waiting for service to start..."
sleep 2

echo "4. Checking service status..."

if systemctl is-active --quiet async-tcp-udp-server; then
    echo "✓ Service is active (as expected)"
    systemctl status async-tcp-udp-server --no-pager
else
    echo "✗ Service failed to start!"
    systemctl status async-tcp-udp-server --no-pager
    exit 1
fi

echo "5. Testing TCP commands:"
echo "/time" | /usr/local/bin/async_client tcp 127.0.0.1 8080
echo "/stats" | /usr/local/bin/async_client tcp 127.0.0.1 8080

echo "6. Testing UDP commands:"
echo "/time" | /usr/local/bin/async_client udp 127.0.0.1 8080

echo "7. Stopping service..."
systemctl stop async-tcp-udp-server

echo "8. Checking service stopped gracefully..."

if ! systemctl is-active --quiet async-tcp-udp-server; then
    echo "✓ Service is inactive (as expected after stop)"
    systemctl status async-tcp-udp-server --no-pager || true
else
    echo "✗ Service is still active after stop command!"
    systemctl status async-tcp-udp-server --no-pager
    exit 1
fi

echo "=== Systemd Test Complete Successfully ==="
exit 0