#!/bin/bash

set -e

echo "=== Statistics Test with Netcat ==="

# Запускаем сервер
./build/async_tcp_udp_server 8080 > server.log 2>&1 &
SERVER_PID=$!
sleep 2

echo "1. Initial stats:"
echo "/stats" | nc -w 1 127.0.0.1 8080

echo ""
echo "2. Three sequential connections via netcat:"
for i in {1..3}; do
    echo "Hello$i" | nc -w 1 127.0.0.1 8080
    echo "Stats after $i:"
    echo "/stats" | nc -w 1 127.0.0.1 8080
    echo ""
done

echo "3. Final stats (should be total=3, current=0):"
echo "/stats" | nc -w 1 127.0.0.1 8080

echo ""
echo "4. Shutdown:"
echo "/shutdown" | nc -w 1 127.0.0.1 8080

sleep 2
cat server.log
rm -f server.log

echo "✅ Test completed"