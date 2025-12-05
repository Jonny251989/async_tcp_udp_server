FROM ubuntu:24.04

ENV TZ=Europe/Moscow
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y \
    systemd \
    tzdata \
    iproute2 \
    net-tools \
    lsof \
    curl \
    g++ \
    make \
    cmake \
    libgtest-dev \
    libgmock-dev \
    netcat-openbsd \
    iputils-ping \
    sudo \
    dbus \
    procps \
    && rm -rf /var/lib/apt/lists/*

RUN cd /usr/src/googletest && \
    mkdir -p build && cd build && \
    cmake .. && \
    make -j$(nproc) && \
    make install

RUN rm -f /lib/systemd/system/getty.target

WORKDIR /workspace

COPY . /workspace/

RUN make all

RUN cp build/async_tcp_udp_server /usr/bin/ && \
    cp build/client_app /usr/bin/ && \
    chmod +x /usr/bin/async_tcp_udp_server /usr/bin/client_app

RUN useradd -r -s /bin/false async-server && \
    echo 'async-server ALL=(ALL) NOPASSWD: ALL' >> /etc/sudoers

RUN echo '[Unit]' > /etc/systemd/system/async-server.service && \
    echo 'Description=Async TCP/UDP Server' >> /etc/systemd/system/async-server.service && \
    echo 'After=network.target' >> /etc/systemd/system/async-server.service && \
    echo '' >> /etc/systemd/system/async-server.service && \
    echo '[Service]' >> /etc/systemd/system/async-server.service && \
    echo 'Type=simple' >> /etc/systemd/system/async-server.service && \
    echo 'User=async-server' >> /etc/systemd/system/async-server.service && \
    echo 'Group=async-server' >> /etc/systemd/system/async-server.service && \
    echo 'ExecStart=/usr/bin/async_tcp_udp_server 8080' >> /etc/systemd/system/async-server.service && \
    echo 'Restart=on-failure' >> /etc/systemd/system/async-server.service && \
    echo 'RestartSec=5' >> /etc/systemd/system/async-server.service && \
    echo 'StandardOutput=journal' >> /etc/systemd/system/async-server.service && \
    echo '' >> /etc/systemd/system/async-server.service && \
    echo '[Install]' >> /etc/systemd/system/async-server.service && \
    echo 'WantedBy=multi-user.target' >> /etc/systemd/system/async-server.service

CMD ["/bin/bash"]