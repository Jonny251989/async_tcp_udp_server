.PHONY: all clean install uninstall systemd-install systemd-uninstall test unit-test functional-test systemd-test all-tests clean-test clean-all help

CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -pthread -O2 -I. -Icommon -Iserver -Iclient
LDFLAGS = -pthread

BUILD_DIR = build
SERVER_SRC_DIR = server
CLIENT_SRC_DIR = client
TEST_SRC_DIR = tests

SERVER_SRCS = $(SERVER_SRC_DIR)/main.cpp \
              $(SERVER_SRC_DIR)/server.cpp \
              $(SERVER_SRC_DIR)/tcp_handler.cpp \
              $(SERVER_SRC_DIR)/udp_handler.cpp \
              $(SERVER_SRC_DIR)/tcp_connection.cpp \
              $(SERVER_SRC_DIR)/command_processor.cpp \
              $(SERVER_SRC_DIR)/eventloop.cpp \
              $(SERVER_SRC_DIR)/command.cpp \
              $(SERVER_SRC_DIR)/session_manager.cpp

SERVER_OBJS = $(SERVER_SRCS:%.cpp=$(BUILD_DIR)/%.o)

CLIENT_SRCS = $(CLIENT_SRC_DIR)/main.cpp
CLIENT_OBJS = $(CLIENT_SRCS:%.cpp=$(BUILD_DIR)/%.o)

UNIT_TEST_SRCS = $(TEST_SRC_DIR)/unit/test_main.cpp \
                 $(TEST_SRC_DIR)/unit/test_command_processor.cpp \
                 $(TEST_SRC_DIR)/unit/test_session_manager.cpp 
UNIT_TEST_OBJS = $(UNIT_TEST_SRCS:%.cpp=$(BUILD_DIR)/%.o)

all: $(BUILD_DIR)/async_tcp_udp_server $(BUILD_DIR)/client_app

$(BUILD_DIR)/async_tcp_udp_server: $(SERVER_OBJS)
	@mkdir -p $(@D)
	$(CXX) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/client_app: $(CLIENT_OBJS)
	@mkdir -p $(@D)
	$(CXX) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

install: all
	install -d /usr/local/bin
	install -m 755 $(BUILD_DIR)/async_tcp_udp_server /usr/local/bin/
	install -m 755 $(BUILD_DIR)/client_app /usr/local/bin/async_client
	install -d /etc/async-tcp-udp-server
	install -m 644 config/server.conf.example /etc/async-tcp-udp-server/server.conf 2>/dev/null || true
	@echo "Server installed to /usr/local/bin/async_tcp_udp_server"
	@echo "Client installed to /usr/local/bin/async_client"
	@echo "Config: /etc/async-tcp-udp-server/server.conf"


uninstall:
	systemctl stop async-tcp-udp-server 2>/dev/null || true
	systemctl disable async-tcp-udp-server 2>/dev/null || true
	rm -f /lib/systemd/system/async-tcp-udp-server.service
	systemctl daemon-reload
	rm -f /usr/local/bin/async_tcp_udp_server
	rm -f /usr/local/bin/async_client
	rm -rf /etc/async-tcp-udp-server
	@echo "Server uninstalled"

systemd-install:
	@echo "Installing systemd service..."
	install -d /lib/systemd/system
	install -m 644 systemd/async-tcp-udp-server.service /lib/systemd/system/
	@echo "Creating system user..."
	@if ! id async-server >/dev/null 2>&1; then \
		useradd -r -s /usr/sbin/nologin async-server 2>/dev/null || true; \
	fi
	@echo "Enabling service..."
	systemctl daemon-reload
	systemctl enable async-tcp-udp-server
	@echo "Systemd service installed. Start with: systemctl start async-tcp-udp-server"

TEST_BUILD_DIR = $(BUILD_DIR)/tests


.PHONY: test unit-test functional-test systemd-test all-tests

test: unit-test functional-test
	@echo "=== Unit and Functional Tests PASSED ==="

all-tests: test systemd-test
	@echo "=== ALL TESTS PASSED ==="

unit-test: $(TEST_BUILD_DIR)/unit_tests
	@echo "=== Running unit tests ==="
	@$(TEST_BUILD_DIR)/unit_tests

functional-test: $(BUILD_DIR)/async_tcp_udp_server $(BUILD_DIR)/client_app
	@echo "=== Running functional tests (no sudo) ==="
	@bash $(TEST_SRC_DIR)/functional/test_server.sh

systemd-test: $(BUILD_DIR)/async_tcp_udp_server $(BUILD_DIR)/client_app
	@echo "=== Running systemd integration tests (requires sudo) ==="
	@sudo bash $(TEST_SRC_DIR)/functional/systemd_test.sh

$(TEST_BUILD_DIR)/unit_tests: $(UNIT_TEST_OBJS) \
                              $(BUILD_DIR)/server/command.o \
                              $(BUILD_DIR)/server/session_manager.o \
                              $(BUILD_DIR)/server/command_processor.o
	@mkdir -p $(TEST_BUILD_DIR)
	@echo "Building unit tests..."
	$(CXX) $(CXXFLAGS) $^ -lgtest -lgtest_main -lpthread -o $@

clean-test:
	rm -rf $(TEST_BUILD_DIR)

clean-all: clean clean-test

help:
	@echo "Available targets:"
	@echo "  all               - Build server and client"
	@echo "  clean             - Clean build directory"
	@echo "  install           - Install binaries and configs"
	@echo "  uninstall         - Uninstall everything"
	@echo "  systemd-install   - Install systemd service"
	@echo ""
	@echo "Testing targets:"
	@echo "  test              - Run unit + functional tests (no sudo)"
	@echo "  unit-test         - Run unit tests only"
	@echo "  functional-test   - Run functional tests (bash script, no sudo)"
	@echo "  systemd-test      - Run systemd tests (requires sudo)"
	@echo "  all-tests         - Run ALL tests (unit + functional + systemd)"
	@echo "  clean-test        - Clean test build directory"
	@echo "  clean-all         - Clean everything"