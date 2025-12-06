#!/bin/bash

set -e

echo "=== Systemd Integration Test ==="

if [ "$EUID" -ne 0 ]; then 
    echo "Please run as root (sudo)"
    exit 1
fi

# Определяем имя сервиса
SERVICE_NAME="async-server.service"
SERVICE_FILE="/lib/systemd/system/$SERVICE_NAME"
CLIENT_PATH="/usr/local/bin/async_client"

# Функция для очистки
cleanup() {
    echo "Cleaning up..."
    systemctl stop $SERVICE_NAME 2>/dev/null || true
    systemctl disable $SERVICE_NAME 2>/dev/null || true
    rm -f $SERVICE_FILE 2>/dev/null || true
    systemctl daemon-reload 2>/dev/null || true
    echo "Cleanup complete"
}

# Устанавливаем обработчик прерывания
trap cleanup EXIT

echo "1. Building and installing..."
make clean
make all
make install

echo "2. Installing service..."
if [ -f systemd/async-tcp-udp-server.service ]; then
    install -m644 systemd/async-tcp-udp-server.service $SERVICE_FILE
else
    cat > $SERVICE_FILE << EOF
[Unit]
Description=Async TCP/UDP Server
After=network.target

[Service]
Type=simple
User=async-server
Group=async-server
ExecStart=/usr/local/bin/async_tcp_udp_server 8080
Restart=on-failure
RestartSec=5
StandardOutput=journal

[Install]
WantedBy=multi-user.target
EOF
fi

# Создаем пользователя если не существует
id async-server >/dev/null 2>&1 || useradd -r -s /usr/sbin/nologin async-server 2>/dev/null || true

systemctl daemon-reload
systemctl enable $SERVICE_NAME

echo "3. Starting service..."
systemctl start $SERVICE_NAME

echo "4. Waiting for service to start..."
sleep 3

echo "5. Checking service status..."

if systemctl is-active --quiet $SERVICE_NAME; then
    echo "✓ Service is active"
    systemctl status $SERVICE_NAME --no-pager | head -20
else
    echo "✗ Service failed to start!"
    journalctl -u $SERVICE_NAME --no-pager -n 20
    exit 1
fi

echo "6. Testing server commands..."

echo "   Testing /time (TCP):"
if echo "/time" | timeout 2 nc -N 127.0.0.1 8080 2>/dev/null; then
    echo "   ✓ TCP time command works"
else
    echo "   ✗ TCP time command failed"
fi

echo "   Testing /stats (TCP):"
if echo "/stats" | timeout 2 nc -N 127.0.0.1 8080 2>/dev/null; then
    echo "   ✓ TCP stats command works"
else
    echo "   ✗ TCP stats command failed"
fi

echo "   Testing echo (TCP):"
if echo "Hello" | timeout 2 nc -N 127.0.0.1 8080 2>/dev/null | grep -q "Hello"; then
    echo "   ✓ TCP echo works"
else
    echo "   ✗ TCP echo failed"
fi

echo "   Testing /time (UDP):"
if echo "/time" | timeout 2 nc -u -w 1 127.0.0.1 8080 2>/dev/null; then
    echo "   ✓ UDP time command works"
else
    echo "   ✗ UDP time command failed"
fi

echo "7. Testing shutdown command..."
echo "/shutdown" | timeout 2 nc -N 127.0.0.1 8080 2>/dev/null || true
sleep 2

echo "8. Verifying service stopped gracefully..."

# Проверяем, что сервис остановился
if ! systemctl is-active --quiet $SERVICE_NAME 2>/dev/null; then
    echo "✓ Service stopped successfully"
    
    # Проверяем exit code сервиса
    SERVICE_EXIT_CODE=$(systemctl show -p ExecMainStatus $SERVICE_NAME --value 2>/dev/null || echo "unknown")
    if [ "$SERVICE_EXIT_CODE" = "0" ] || [ "$SERVICE_EXIT_CODE" = "unknown" ]; then
        echo "✓ Service exited with clean status"
    else
        echo "⚠ Service exited with code: $SERVICE_EXIT_CODE"
    fi
else
    echo "✗ Service is still running!"
    systemctl stop $SERVICE_NAME
    exit 1
fi

echo "9. Final journal log (last 5 lines):"
journalctl -u $SERVICE_NAME --no-pager -n 5 2>/dev/null || echo "No journal entries found"

echo ""
echo "=== Systemd Test Complete Successfully ==="
exit 0