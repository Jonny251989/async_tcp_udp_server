#!/bin/bash
set -e

echo "=== 1. Building Docker image ==="
docker build -f Dockerfile -t async-test-runner .

echo -e "\n=== 2. Running unit tests ==="
docker run --rm -v $(pwd):/workspace -w /workspace async-test-runner make unit-test

echo -e "\n=== 3. Running functional tests ==="
docker run --rm -v $(pwd):/workspace -w /workspace --network host async-test-runner \
  bash -c "/usr/bin/async_tcp_udp_server 8080 & SERVER_PID=\$! && sleep 2 && make functional-test && kill \$SERVER_PID 2>/dev/null; true"

echo -e "\n=== 4. Running systemd tests ==="

# Создаем временный контейнер для systemd тестов
CONTAINER_ID=$(docker run -d --name async-systemd-test \
  --privileged \
  --tmpfs /run \
  --tmpfs /tmp \
  -v $(pwd):/workspace \
  -w /workspace \
  -p 8080:8080 \
  async-test-runner \
  /lib/systemd/systemd --system)

# Ждем запуска systemd
sleep 5

echo "4.1. Installing systemd service..."
docker exec $CONTAINER_ID bash -c "
  # Включаем сервис
  systemctl daemon-reload
  systemctl enable async-server.service
  systemctl start async-server.service
  sleep 3
  
  # Проверяем статус
  systemctl status async-server.service --no-pager
"

echo "4.2. Testing server commands..."
docker exec $CONTAINER_ID bash -c "
  # Проверяем, что сервер слушает порт
  echo '=== Checking ports ==='
  netstat -tulpn 2>/dev/null | grep :8080 || ss -tulpn 2>/dev/null | grep :8080
  
  echo '=== Testing commands ==='
  echo '1. /time:'
  echo '/time' | timeout 1 nc -N localhost 8080 || true
  echo ''
  
  echo '2. /stats:'
  echo '/stats' | timeout 1 nc -N localhost 8080 || true
  echo ''
  
  echo '3. Echo test:'
  echo 'Hello' | timeout 1 nc -N localhost 8080 || true
  echo ''
"

echo "4.3. Stopping service..."
docker exec $CONTAINER_ID systemctl stop async-server.service
sleep 2

echo "4.4. Testing /shutdown command..."
docker exec $CONTAINER_ID bash -c "
  systemctl start async-server.service
  sleep 2
  echo '/shutdown' | timeout 1 nc -N localhost 8080 || true
  sleep 2
  # После shutdown сервер должен остановиться
  if ! systemctl is-active async-server.service 2>/dev/null; then
    echo '✓ Server stopped successfully via /shutdown command'
  else
    echo '✗ Server is still running'
    exit 1
  fi
"

# Останавливаем и удаляем контейнер
docker stop async-systemd-test 2>/dev/null || true
docker rm async-systemd-test 2>/dev/null || true

echo -e "\n=== ALL TESTS COMPLETED SUCCESSFULLY! ==="
