CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -pthread -O2 -Icommon -Iserver
LDFLAGS = -pthread

# Directories
COMMON_DIR = common
SERVER_DIR = server
CLIENT_DIR = client
BUILD_DIR = build
SYSTEMD_DIR = systemd
CONFIG_DIR = config
SCRIPTS_DIR = scripts

# Source files - session_manager перемещен в server
SERVER_SOURCES = $(SERVER_DIR)/main.cpp $(SERVER_DIR)/server.cpp \
                 $(SERVER_DIR)/tcp_handler.cpp $(SERVER_DIR)/udp_handler.cpp \
                 $(SERVER_DIR)/tcp_connection.cpp $(SERVER_DIR)/command_processor.cpp \
                 $(SERVER_DIR)/eventloop.cpp \
                 $(SERVER_DIR)/command.cpp \
                 $(SERVER_DIR)/session_manager.cpp  # ← ПЕРЕМЕЩЕН СЮДА

CLIENT_SOURCES = $(CLIENT_DIR)/main.cpp

# Object files
SERVER_OBJS = $(patsubst $(SERVER_DIR)/%.cpp,$(BUILD_DIR)/server/%.o,$(SERVER_SOURCES))
CLIENT_OBJS = $(patsubst $(CLIENT_DIR)/%.cpp,$(BUILD_DIR)/client/%.o,$(CLIENT_SOURCES))

# Targets
TARGET_SERVER = $(BUILD_DIR)/async_tcp_udp_server
TARGET_CLIENT = $(BUILD_DIR)/client_app

# Package info
PKG_NAME = async-tcp-udp-server
PKG_VERSION = 1.0.0
PKG_ARCH = amd64
DEB_DIR = $(BUILD_DIR)/deb
DEB_PKG = $(PKG_NAME)_$(PKG_VERSION)_$(PKG_ARCH).deb

.PHONY: all clean install uninstall package systemd-install systemd-uninstall env-test

all: $(TARGET_SERVER) $(TARGET_CLIENT)

$(TARGET_SERVER): $(SERVER_OBJS)
	@mkdir -p $(@D)
	$(CXX) $^ -o $@ $(LDFLAGS)

$(TARGET_CLIENT): $(CLIENT_OBJS)
	@mkdir -p $(@D)
	$(CXX) $^ -o $@

$(BUILD_DIR)/server/%.o: $(SERVER_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/client/%.o: $(CLIENT_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

# Installation
install: $(TARGET_SERVER)
	install -d /usr/local/bin
	install -m 755 $(TARGET_SERVER) /usr/local/bin/
	install -m 755 $(TARGET_CLIENT) /usr/local/bin/async_client
	install -d /etc/$(PKG_NAME)
	install -m 644 $(CONFIG_DIR)/server.conf.example /etc/$(PKG_NAME)/server.conf 2>/dev/null || true
	@echo "Server installed to /usr/local/bin/async_tcp_udp_server"
	@echo "Client installed to /usr/local/bin/async_client"
	@echo "Config: /etc/$(PKG_NAME)/server.conf"

# Systemd installation
systemd-install: $(TARGET_SERVER)
	@echo "Installing systemd service..."
	install -d /lib/systemd/system
	install -m 644 $(SYSTEMD_DIR)/async-tcp-udp-server.service /lib/systemd/system/
	@echo "Creating system user..."
	@-useradd -r -s /bin/false -d /opt/$(PKG_NAME) -M async-server 2>/dev/null || true
	@mkdir -p /opt/$(PKG_NAME) /var/log/$(PKG_NAME)
	@chown async-server:async-server /opt/$(PKG_NAME) /var/log/$(PKG_NAME)
	@echo "Enabling service..."
	-systemctl daemon-reload
	-systemctl enable async-tcp-udp-server
	@echo "Systemd service installed. Start with: systemctl start async-tcp-udp-server"

# Systemd uninstall
systemd-uninstall:
	-systemctl stop async-tcp-udp-server 2>/dev/null || true
	-systemctl disable async-tcp-udp-server 2>/dev/null || true
	-rm -f /lib/systemd/system/async-tcp-udp-server.service
	-systemctl daemon-reload
	@echo "Systemd service uninstalled"

# Debian package
package: clean all
	@echo "Building Debian package..."
	@rm -rf $(DEB_DIR)
	@mkdir -p $(DEB_DIR)/DEBIAN
	@mkdir -p $(DEB_DIR)/usr/local/bin
	@mkdir -p $(DEB_DIR)/lib/systemd/system
	@mkdir -p $(DEB_DIR)/etc/$(PKG_NAME)
	@mkdir -p $(DEB_DIR)/opt/$(PKG_NAME)
	
	# Copy binaries
	install -m 755 $(TARGET_SERVER) $(DEB_DIR)/usr/local/bin/async_tcp_udp_server
	install -m 755 $(TARGET_CLIENT) $(DEB_DIR)/usr/local/bin/async_client
	install -m 644 $(SYSTEMD_DIR)/async-tcp-udp-server.service $(DEB_DIR)/lib/systemd/system/
	install -m 644 $(CONFIG_DIR)/server.conf.example $(DEB_DIR)/etc/$(PKG_NAME)/server.conf
	
	# Control file
	@cp debian/control $(DEB_DIR)/DEBIAN/
	@cp debian/postinst $(DEB_DIR)/DEBIAN/ && chmod 755 $(DEB_DIR)/DEBIAN/postinst
	@cp debian/prerm $(DEB_DIR)/DEBIAN/ && chmod 755 $(DEB_DIR)/DEBIAN/prerm
	
	# Build package
	dpkg-deb --build $(DEB_DIR) $(DEB_PKG)
	@echo "Package built: $(DEB_PKG)"

# Uninstall
uninstall: systemd-uninstall
	-rm -f /usr/local/bin/async_tcp_udp_server
	-rm -f /usr/local/bin/async_client
	-rm -rf /etc/$(PKG_NAME)
	@echo "Server uninstalled"

# Environment variables test
env-test:
	@echo "=== Environment Variables Test ==="
	@echo "Current environment:"
	@echo "SERVER_PORT=$${SERVER_PORT:-not set}"
	@echo "LOG_LEVEL=$${LOG_LEVEL:-not set}"
	@echo ""
	@echo "To set environment:"
	@echo "  export SERVER_PORT=9090"
	@echo "  export LOG_LEVEL=debug"
	@echo "  ./$(TARGET_SERVER)"

# Service management
status:
	systemctl status async-tcp-udp-server

logs:
	journalctl -u async-tcp-udp-server -f

restart:
	systemctl restart async-tcp-udp-server

stop:
	systemctl stop async-tcp-udp-server

start:
	systemctl start async-tcp-udp-server

quick_test: $(TARGET_SERVER) $(TARGET_CLIENT)
	@echo "Starting quick test..."
	@./scripts/test_server.sh

help:
	@echo "Available targets:"
	@echo "  all              - Build everything"
	@echo "  clean            - Clean build"
	@echo "  install          - Install to system"
	@echo "  uninstall        - Uninstall from system"
	@echo "  systemd-install  - Install systemd service"
	@echo "  systemd-uninstall - Uninstall systemd service"
	@echo "  package          - Build Debian package"
	@echo "  env-test         - Test environment variables"
	@echo "  status|logs|restart|start|stop - Service management"
	@echo "  quick_test       - Run quick test"