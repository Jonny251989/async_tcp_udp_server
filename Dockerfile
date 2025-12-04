FROM ubuntu:22.04

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
    netcat \
    netcat-openbsd \
    net-tools \
    iputils-ping \
    curl \
    && rm -rf /var/lib/apt/lists/*

RUN systemctl mask \
    systemd-tmpfiles-setup.service \
    systemd-tmpfiles-setup-dev.service \
    systemd-tmpfiles-clean.service \
    systemd-firstboot.service \
    systemd-logind.service \
    systemd-udevd.service

RUN cd /usr/src/gtest && \
    cmake CMakeLists.txt && \
    make && \
    find . -name '*.a' -exec cp {} /usr/lib \; && \
    rm -rf /usr/src/gtest

WORKDIR /workspace
COPY . /workspace

RUN make clean && make all
RUN make install

RUN useradd -r -s /usr/sbin/nologin async-server

RUN mkdir -p /etc/async-tcp-udp-server /var/log/async-tcp-udp-server && \
    chown -R async-server:async-server /etc/async-tcp-udp-server /var/log/async-tcp-udp-server

RUN echo -e "# Async TCP/UDP Server Configuration\nport=8080\nlog_level=info" > /etc/async-tcp-udp-server/server.conf && \
    chown async-server:async-server /etc/async-tcp-udp-server/server.conf

COPY deploy/systemd/async-tcp-udp-server.service /etc/systemd/system/

RUN systemctl enable async-tcp-udp-server.service

RUN echo '#!/bin/bash\nsystemctl is-active async-tcp-udp-server' > /usr/local/bin/healthcheck.sh && \
    chmod +x /usr/local/bin/healthcheck.sh

EXPOSE 8080/tcp 8080/udp

CMD ["/sbin/init"]