FROM ubuntu:22.04

# Установка systemd и зависимостей
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y \
    build-essential \
    cmake \
    g++ \
    make \
    libgtest-dev \
    libgmock-dev \
    systemd \
    systemd-sysv \
    sudo \
    dbus \
    && rm -rf /var/lib/apt/lists/*

# Настройка systemd для контейнеров
RUN systemctl mask \
    systemd-tmpfiles-setup.service \
    systemd-tmpfiles-setup-dev.service \
    systemd-tmpfiles-clean.service \
    systemd-firstboot.service \
    systemd-logind.service \
    systemd-udevd.service

# Установка Google Test
RUN cd /usr/src/gtest && \
    cmake CMakeLists.txt && \
    make && \
    find . -name '*.a' -exec cp {} /usr/lib \; && \
    rm -rf /usr/src/gtest

# Создание рабочей директории
WORKDIR /workspace

# Копируем исходный код
COPY . /workspace

# Собираем проект
RUN make clean && make all

# Устанавливаем сервер и клиент
RUN make install

# Создаем пользователя
RUN id async-server || useradd -r -s /usr/sbin/nologin async-server

# Создаем директории
RUN mkdir -p /etc/async-tcp-udp-server /var/log/async-tcp-udp-server && \
    chown -R async-server:async-server /etc/async-tcp-udp-server /var/log/async-tcp-udp-server

# Создаем дефолтный конфиг
RUN echo -e "# Async TCP/UDP Server Configuration\nport=8080\nlog_level=info" > /etc/async-tcp-udp-server/server.conf && \
    chown async-server:async-server /etc/async-tcp-udp-server/server.conf

# Копируем systemd unit файл из проекта
COPY deploy/systemd/async-tcp-udp-server.service /etc/systemd/system/

# Включаем сервис
RUN systemctl enable async-tcp-udp-server.service

EXPOSE 8080/tcp 8080/udp

# Запускаем systemd
CMD ["/sbin/init"]