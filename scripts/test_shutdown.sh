#!/bin/bash

echo "=== Detailed Command Debug ==="

# Останавливаем сервер
sudo systemctl stop async-tcp-udp-server 2>/dev/null
sleep 2

# Запускаем сервер с подробным логированием
echo "Starting server with debug logging..."
sudo -u async-server /usr/local/bin/async_tcp_udp_server 8080 2>&1 | tee server_debug.log &
SERVER_PID=$!
echo "Server PID: $SERVER_PID"

# Ждем запуска
sleep 3

echo ""
echo "1. Testing if server is responsive..."
echo "PING" | timeout 2 nc -w 2 127.0.0.1 8080
echo ""

echo "2. Testing /time command:"
echo "/time" | timeout 2 nc -w 2 127.0.0.1 8080
echo ""

echo "3. Testing /stats command:"
echo "/stats" | timeout 2 nc -w 2 127.0.0.1 8080
echo ""

echo "4. Testing /shutdown command with echo:"
echo "Sending: /shutdown"
echo "/shutdown" | timeout 2 nc -w 2 -v 127.0.0.1 8080 2>&1
echo "Netcat finished"

# Проверяем сервер
sleep 3
if kill -0 $SERVER_PID 2>/dev/null; then
    echo "❌ Server still running. Checking process..."
    ps -p $SERVER_PID -o pid,cmd
    echo "Killing server..."
    kill $SERVER_PID
else
    echo "✅ Server stopped"
fi

# Выводим лог сервера
echo ""
echo "=== Server Debug Log ==="
cat server_debug.log
rm -f server_debug.log