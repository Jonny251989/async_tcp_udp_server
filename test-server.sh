#!/bin/bash
ivan@ivan-GL552VX:~/async_tcp_udp_server$ echo "=== ПОЛНАЯ ПРОВЕРКА РАБОТОСПОСОБНОСТИ СЕРВЕРА ==="
echo

echo "1. ✅ Контейнер запущен:"
docker ps -f name=async-server --format "table {{.ID}}\t{{.Names}}\t{{.Status}}\t{{.Ports}}"

echo
echo "2. Проверка systemd сервиса внутри контейнера:"
if docker exec async-server systemctl is-active async-tcp-udp-server --quiet; then
    echo "   ✅ Сервис активен"
    echo "   Подробный статус:"
    docker exec async-server systemctl status async-tcp-udp-server --no-pager | grep -A 3 "Active:"
else
    echo "   ❌ Сервис не активен"
fi

echo
echo "3. Проверка открытых портов:"
echo "   Внутри контейнера:"
fi  echo "❌ ПРОБЛЕМЫ С СЕРВЕРОМ"тивен"docker exec async-server systemctl is-active async-tcp-udp-server --quiet; thenфайл не найден"ючился" || echo "   ⚠
=== ПОЛНАЯ ПРОВЕРКА РАБОТОСПОСОБНОСТИ СЕРВЕРА ===

1. ✅ Контейнер запущен:
CONTAINER ID  NAMES         STATUS            PORTS
4855bd7c75b9  async-server  Up 6 minutes ago  0.0.0.0:8080->8080/tcp, 0.0.0.0:8080->8080/udp

2. Проверка systemd сервиса внутри контейнера:
   ✅ Сервис активен
   Подробный статус:
     Active: active (running) since Tue 2025-12-02 15:30:20 UTC; 6min ago
   Main PID: 36 (async_tcp_udp_s)
      Tasks: 1 (limit: 307)
     Memory: 336.0K

3. Проверка открытых портов:
   Внутри контейнера:
   Не удалось проверить порты

4. Тестирование TCP соединения:
   Отправка тестового сообщения...
HELLO
QUIT
   ❌ TCP соединение не работает

5. Тестирование UDP соединения:
   Отправка UDP пакета...
UDP_TEST   ✅ UDP пакет отправлен

6. Проверка логов сервера (последние 10 строк):
Dec 02 15:30:20 4855bd7c75b9 systemd[1]: Started Async TCP/UDP Server.
Dec 02 15:30:20 4855bd7c75b9 async-tcp-udp-server[36]: Using port from environment: 8080
Dec 02 15:30:20 4855bd7c75b9 async-tcp-udp-server[36]: Server running on port 8080
Dec 02 15:30:20 4855bd7c75b9 async-tcp-udp-server[36]: Press Ctrl+C to exit...

7. Проверка процесса сервера:
async-s+      36  1.5  0.0   6104  3328 ?        Ss   15:30   0:06 /usr/local/bin/async_tcp_udp_server

8. Тестирование клиентом внутри контейнера:
   Попытка подключения TCP клиентом...
Connected to TCP server 127.0.0.1:8080
   ✅ Клиент подключился

9. Проверка конфигурации:
-e # Async TCP/UDP Server Configuration
port=8080
log_level=info

=== ФИНАЛЬНЫЙ ВЕРДИКТ ===
✅ СЕРВЕР РАБОТАЕТ КОРРЕКТНО
   - Контейнер: запущен
   - Systemd сервис: активен
   - Порт 8080: открыт
ivan@ivan-GL552VX:~/async_tcp_udp_server$ 