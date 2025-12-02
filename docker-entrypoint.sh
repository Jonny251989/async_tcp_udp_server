#!/bin/bash
set -e

echo "=== Docker Entrypoint ==="
echo "Mode: $1"

case "$1" in
    systemd|"")
        echo "Starting with systemd..."
        
        # Проверяем и включаем сервис
        if [ -f "/etc/systemd/system/async-tcp-udp-server.service" ]; then
            echo "Service file found, enabling it..."
            systemctl enable async-tcp-udp-server.service 2>/dev/null || true
        else
            echo "WARNING: Service file not found!"
        fi
        
        # Запускаем systemd
        exec /sbin/init
        ;;
        
    direct)
        echo "Starting server directly..."
        PORT=8080
        CONFIG_FILE="/etc/async-tcp-udp-server/server.conf"
        
        if [ -f "$CONFIG_FILE" ]; then
            EXTRACTED_PORT=$(grep -E '^port=' "$CONFIG_FILE" | cut -d= -f2 | tr -d '[:space:]')
            if [ -n "$EXTRACTED_PORT" ]; then
                PORT="$EXTRACTED_PORT"
            fi
        fi
        
        echo "Server port: $PORT"
        # Запускаем от имени async-server
        exec sudo -u async-server /usr/local/bin/async_tcp_udp_server "$PORT"
        ;;
        
    test)
        echo "Testing server..."
        /usr/local/bin/async_tcp_udp_server 8080
        ;;
        
    bash)
        exec /bin/bash
        ;;
        
    *)
        echo "Unknown command: $1"
        echo "Available commands: systemd, direct, test, bash"
        echo "Starting with systemd by default..."
        exec /sbin/init
        ;;
esac