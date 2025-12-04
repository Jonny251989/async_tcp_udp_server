#!/bin/bash

echo "=== Тестирование Async TCP/UDP Server ==="
echo

echo "1. Статус контейнера:"
docker ps | grep async-server

echo
echo "2. Статус systemd сервиса:"
docker exec async-server systemctl status async-tcp-udp-server --no-pager | head -10

echo
echo "3. Проверка портов в контейнере:"
docker exec async-server ss -tulpn | grep :8080

echo
echo "4. Тест TCP подключения:"
timeout 2 bash -c "echo -e 'TCP Test Message\nQUIT' | nc localhost 8080" && echo "✓ TCP тест успешен" || echo "✗ TCP тест не удался"

echo
echo "5. Тест UDP подключения:"
echo "UDP Test" | timeout 2 nc -u -w 1 localhost 8080 && echo "✓ UDP тест отправлен" || echo "✗ UDP тест не удался"

echo
echo "6. Логи сервера (последние 5 строк):"
docker exec async-server journalctl -u async-tcp-udp-server --no-pager -n 5

echo
echo "=== Тестирование завершено ==="
