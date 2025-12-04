FROM ubuntu:24.04

# Установите таймзону в начале
ENV TZ=Europe/Moscow
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# Установка systemd и необходимых компонентов
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y \
    systemd \
    tzdata \
    iproute2 \
    net-tools \
    lsof \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Удаляем getty.target (чтобы не было запроса логина)
RUN rm -f /lib/systemd/system/getty.target

# Копируем бинарники
COPY build/async_tcp_udp_server /usr/bin/
COPY build/client_app /usr/bin/
RUN chmod +x /usr/bin/async_tcp_udp_server /usr/bin/client_app

# Пользователь
RUN useradd -r -s /bin/false async-server

# Systemd сервис
RUN echo '[Unit]' > /etc/systemd/system/async-server.service
RUN echo 'Description=Async TCP/UDP Server' >> /etc/systemd/system/async-server.service
RUN echo 'After=network.target' >> /etc/systemd/system/async-server.service
RUN echo '' >> /etc/systemd/system/async-server.service
RUN echo '[Service]' >> /etc/systemd/system/async-server.service
RUN echo 'Type=simple' >> /etc/systemd/system/async-server.service
RUN echo 'User=async-server' >> /etc/systemd/system/async-server.service
RUN echo 'Group=async-server' >> /etc/systemd/system/async-server.service
RUN echo 'ExecStart=/usr/bin/async_tcp_udp_server 8080' >> /etc/systemd/system/async-server.service
RUN echo 'Restart=always' >> /etc/systemd/system/async-server.service
RUN echo 'RestartSec=5' >> /etc/systemd/system/async-server.service
RUN echo '' >> /etc/systemd/system/async-server.service
RUN echo '[Install]' >> /etc/systemd/system/async-server.service
RUN echo 'WantedBy=multi-user.target' >> /etc/systemd/system/async-server.service

# Включаем сервис вручную
RUN mkdir -p /etc/systemd/system/multi-user.target.wants && \
    ln -sf /etc/systemd/system/async-server.service /etc/systemd/system/multi-user.target.wants/

# Entrypoint скрипт для генерации machine-id при запуске
RUN echo '#!/bin/sh' > /entrypoint.sh
RUN echo 'set -e' >> /entrypoint.sh
RUN echo 'rm -f /etc/machine-id' >> /entrypoint.sh
RUN echo 'dbus-uuidgen --ensure' >> /entrypoint.sh
RUN echo 'mkdir -p /run/systemd/system' >> /entrypoint.sh
RUN echo 'exec /lib/systemd/systemd --system' >> /entrypoint.sh
RUN chmod +x /entrypoint.sh

# Настройка для Docker
ENV container=docker
STOPSIGNAL SIGRTMIN+3

CMD ["/entrypoint.sh"]