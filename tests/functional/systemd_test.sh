#!/bin/bash

set -e

echo "=== Systemd Integration Test ==="

if [ "$EUID" -ne 0 ]; then 
    echo "Please run as root (sudo)"
    exit 1
fi

# Определяем имя сервиса
SERVICE_NAME="async-server.service"
CLIENT_PATH="/usr/local/bin/async_client"

# Проверяем есть ли клиент
if [ ! -f "$CLIENT_PATH" ]; then
    echo "Installing client..."
    make install
fi

echo "1. Stopping existing service..."
systemctl stop $SERVICE_NAME 2>/dev/null || true
systemctl disable $SERVICE_NAME 2>/dev/null || true

echo "2. Installing service..."
make systemd-install

echo "3. Starting service..."
systemctl daemon-reload
systemctl enable $SERVICE_NAME
systemctl start $SERVICE_NAME

echo "4. Waiting for service to start..."
sleep 3

echo "5. Checking service status..."

if systemctl is-active --quiet $SERVICE_NAME; then
    echo "✓ Service is active (as expected)"
    systemctl status $SERVICE_NAME --no-pager
else
    echo "✗ Service failed to start!"
    systemctl status $SERVICE_NAME --no-pager
    exit 1
fi

echo "6. Testing server commands..."

echo "   Testing /time (TCP):"
echo "/time" | timeout 2 nc -N 127.0.0.1 8080 || echo "Timeout or error"

echo "   Testing /stats (TCP):"
echo "/stats" | timeout 2 nc -N 127.0.0.1 8080 || echo "Timeout or error"

echo "   Testing echo (TCP):"
echo "Hello" | timeout 2 nc -N 127.0.0.1 8080 || echo "Timeout or error"

echo "   Testing /time (UDP):"
echo "/time" | timeout 2 nc -u -w 1 127.0.0.1 8080 || echo "Timeout or error"

echo "7. Testing shutdown command..."
echo "/shutdown" | timeout 2 nc -N 127.0.0.1 8080 || echo "Timeout or error"
sleep 2

echo "8. Checking service stopped gracefully..."

if ! systemctl is-active --quiet $SERVICE_NAME; then
    echo "✓ Service is inactive (as expected after shutdown)"
    systemctl status $SERVICE_NAME --no-pager 2>/dev/null || echo "Service not found (expected after shutdown)"
else
    echo "✗ Service is still active after shutdown command!"
    systemctl status $SERVICE_NAME --no-pager
    systemctl stop $SERVICE_NAME
    exit 1
fi

echo "9. Cleaning up..."
systemctl disable $SERVICE_NAME 2>/dev/null || true
systemctl daemon-reload

echo "=== Systemd Test Complete Successfully ==="
exit 0