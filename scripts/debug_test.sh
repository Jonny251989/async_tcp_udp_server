#!/bin/bash

echo "=== Debug Test ==="

# Перекомпилируем с отладкой
make clean
make all

# Запускаем сервер
./build/async_tcp_udp_server 8080 > server.log 2>&1 &
SERVER_PID=$!
sleep 2

echo "1. First connection:"
echo "/stats" | timeout 2 ./build/client_app tcp 127.0.0.1 8080 2>&1 | grep -E "(DEBUG|Total)"

echo ""
echo "2. Second connection:"
echo "/stats" | timeout 2 ./build/client_app tcp 127.0.0.1 8080 2>&1 | grep -E "(DEBUG|Total)"

echo ""
echo "3. Shutdown:"
echo "/shutdown" | timeout 2 ./build/client_app tcp 127.0.0.1 8080

sleep 1
echo ""
echo "=== Full Debug Output ==="
cat server.log | grep DEBUG

# Убиваем если еще жив
kill $SERVER_PID 2>/dev/null || true