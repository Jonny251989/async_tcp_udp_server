#!/bin/bash

echo "=== Detailed Command Debug ==="

# Останавливаем сервер
sudo systemctl stop async-tcp-udp-server
sleep 2

# Запускаем сервер с подробным логированием
echo "Starting server with debug logging..."
sudo -u async-server /usr/local/bin/async_tcp_udp_server 8080 &
SERVER_PID=$!
echo "Server PID: $SERVER_PID"

# Ждем запуска
sleep 3

echo "1. Testing if server is responsive..."
echo "PING" | nc -w 2 127.0.0.1 8080

echo "2. Testing /time command:"
echo "/time" | nc -w 2 127.0.0.1 8080

echo "3. Testing /stats command:"
echo "/stats" | nc -w 2 127.0.0.1 8080

echo "4. Testing /shutdown command with echo:"
echo "Sending: /shutdown"
echo "/shutdown" | nc -w 2 -v 127.0.0.1 8080
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