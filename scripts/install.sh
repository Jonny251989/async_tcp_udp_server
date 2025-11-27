#!/bin/bash
set -e

echo "=== Installing Async TCP/UDP Server ==="

# Build
make clean
make all

# Install
sudo make install
sudo make systemd-install

echo ""
echo "=== Installation Complete ==="
echo "Service: async-tcp-udp-server"
echo "Config: /etc/async-tcp-udp-server/server.conf"
echo "Binary: /usr/local/bin/async_tcp_udp_server"
echo ""
echo "To start service:"
echo "  sudo systemctl start async-tcp-udp-server"
echo ""
echo "To set custom port:"
echo "  sudo systemctl edit async-tcp-udp-server"