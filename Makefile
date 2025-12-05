.PHONY: all clean install uninstall systemd-install systemd-uninstall \
        test unit-test functional-test systemd-test all-tests clean-test clean-all \
        docker-build docker-test docker-unit docker-functional docker-all docker-clean \
        compose-up compose-unit compose-functional compose-all compose-clean \
        create-service-file check-systemd create-test-script help

# ================================
# КОНФИГУРАЦИЯ
# ================================
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -pthread -O2 -I. -Icommon -Iserver -Iclient
LDFLAGS = -pthread

BUILD_DIR = build
SERVER_SRC_DIR = server
CLIENT_SRC_DIR = client
TEST_SRC_DIR = tests

# Файлы сервера
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

# Файлы клиента
CLIENT_SRCS = $(CLIENT_SRC_DIR)/main.cpp
CLIENT_OBJS = $(CLIENT_SRCS:%.cpp=$(BUILD_DIR)/%.o)

# Unit-тесты
UNIT_TEST_SRCS = $(TEST_SRC_DIR)/unit/test_main.cpp \
                 $(TEST_SRC_DIR)/unit/test_command_processor.cpp \
                 $(TEST_SRC_DIR)/unit/test_session_manager.cpp
UNIT_TEST_OBJS = $(UNIT_TEST_SRCS:%.cpp=$(BUILD_DIR)/%.o)

# ================================
# ОСНОВНЫЕ ЦЕЛИ СБОРКИ
# ================================
all: $(BUILD_DIR)/async_tcp_udp_server $(BUILD_DIR)/client_app
	@echo "=== Build completed ==="

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
	@echo "Build directory cleaned"

# ================================
# УСТАНОВКА И УДАЛЕНИЕ
# ================================
install: all
	install -d /usr/local/bin
	install -m 755 $(BUILD_DIR)/async_tcp_udp_server /usr/local/bin/
	install -m 755 $(BUILD_DIR)/client_app /usr/local/bin/async_client
	install -d /etc/async-tcp-udp-server
	install -m 644 config/server.conf.example /etc/async-tcp-udp-server/server.conf 2>/dev/null || true
	@echo "=== Installation completed ==="
	@echo "Server: /usr/local/bin/async_tcp_udp_server"
	@echo "Client: /usr/local/bin/async_client"
	@echo "Config: /etc/async-tcp-udp-server/server.conf"

uninstall:
	@echo "=== Uninstalling... ==="
	systemctl stop async-server.service 2>/dev/null || true
	systemctl disable async-server.service 2>/dev/null || true
	rm -f /lib/systemd/system/async-server.service 2>/dev/null || true
	systemctl daemon-reload 2>/dev/null || true
	rm -f /usr/local/bin/async_tcp_udp_server 2>/dev/null || true
	rm -f /usr/local/bin/async_client 2>/dev/null || true
	rm -rf /etc/async-tcp-udp-server 2>/dev/null || true
	@echo "=== Uninstallation completed ==="

systemd-install: install
	@echo "=== Installing systemd service ==="
	install -d /lib/systemd/system
	@if [ -f systemd/async-tcp-udp-server.service ]; then \
		install -m 644 systemd/async-tcp-udp-server.service /lib/systemd/system/async-server.service; \
	else \
		echo '[Unit]' > /lib/systemd/system/async-server.service; \
		echo 'Description=Async TCP/UDP Server' >> /lib/systemd/system/async-server.service; \
		echo 'After=network.target' >> /lib/systemd/system/async-server.service; \
		echo '' >> /lib/systemd/system/async-server.service; \
		echo '[Service]' >> /lib/systemd/system/async-server.service; \
		echo 'Type=simple' >> /lib/systemd/system/async-server.service; \
		echo 'User=async-server' >> /lib/systemd/system/async-server.service; \
		echo 'Group=async-server' >> /lib/systemd/system/async-server.service; \
		echo 'ExecStart=/usr/local/bin/async_tcp_udp_server 8080' >> /lib/systemd/system/async-server.service; \
		echo 'Restart=always' >> /lib/systemd/system/async-server.service; \
		echo 'RestartSec=5' >> /lib/systemd/system/async-server.service; \
		echo '' >> /lib/systemd/system/async-server.service; \
		echo '[Install]' >> /lib/systemd/system/async-server.service; \
		echo 'WantedBy=multi-user.target' >> /lib/systemd/system/async-server.service; \
		echo "Created default service file"; \
	fi
	@echo "Creating system user 'async-server'..."
	@if ! id async-server >/dev/null 2>&1; then \
		useradd -r -s /usr/sbin/nologin async-server 2>/dev/null || true; \
		echo "User created"; \
	else \
		echo "User already exists"; \
	fi
	systemctl daemon-reload
	systemctl enable async-server.service
	@echo "=== Systemd service installed ==="
	@echo "Start server: systemctl start async-server.service"
	@echo "Check status: systemctl status async-server.service"

systemd-uninstall: uninstall
	@echo "=== Systemd service removed ==="

# ================================
# ЛОКАЛЬНЫЕ ТЕСТЫ
# ================================
TEST_BUILD_DIR = $(BUILD_DIR)/tests

unit-test: $(TEST_BUILD_DIR)/unit_tests
	@echo "=== Running unit tests ==="
	@$(TEST_BUILD_DIR)/unit_tests
	@echo "=== Unit tests completed ==="

functional-test: $(BUILD_DIR)/async_tcp_udp_server $(BUILD_DIR)/client_app
	@echo "=== Running functional tests ==="
	@bash $(TEST_SRC_DIR)/functional/test_server.sh
	@echo "=== Functional tests completed ==="

systemd-test: all
	@echo "=== Running systemd integration tests (requires sudo) ==="
	@sudo bash $(TEST_SRC_DIR)/functional/systemd_test.sh

test: unit-test functional-test
	@echo "=== All tests passed ==="

all-tests: test
	@echo "=== All tests completed ==="

$(TEST_BUILD_DIR)/unit_tests: $(UNIT_TEST_OBJS) \
                              $(BUILD_DIR)/server/command.o \
                              $(BUILD_DIR)/server/session_manager.o \
                              $(BUILD_DIR)/server/command_processor.o
	@mkdir -p $(TEST_BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -lgtest -lgtest_main -lpthread -o $@

clean-test:
	rm -rf $(TEST_BUILD_DIR)
	@echo "Test build directory cleaned"

clean-all: clean clean-test
	@echo "All build directories cleaned"

# ================================
# DOCKER ТЕСТЫ
# ================================
docker-build:
	@echo "=== Building Docker test image ==="
	docker build -t async-test-runner .
	@echo "=== Docker image built ==="

docker-unit: docker-build
	@echo "=== Running unit tests in Docker ==="
	docker run --rm -v "$(PWD)":/workspace -w /workspace async-test-runner make unit-test

docker-functional: docker-build
	@echo "=== Running functional tests in Docker ==="
	docker run --rm -v "$(PWD)":/workspace -w /workspace --network host async-test-runner \
		bash -c "/usr/bin/async_tcp_udp_server 8080 & \
		SERVER_PID=\$$! && \
		sleep 2 && \
		cd /workspace && \
		make functional-test && \
		kill \$$SERVER_PID 2>/dev/null || true"

docker-test: docker-build
	@echo "=== Running all tests in Docker ==="
	docker run --rm -v "$(PWD)":/workspace -w /workspace async-test-runner make unit-test
	@echo ""
	@echo "=== Running functional tests ==="
	docker run --rm -v "$(PWD)":/workspace -w /workspace --network host async-test-runner \
		bash -c "/usr/bin/async_tcp_udp_server 8080 & \
		SERVER_PID=\$$! && \
		sleep 2 && \
		cd /workspace && \
		make functional-test && \
		kill \$$SERVER_PID 2>/dev/null || true"
	@echo "=== Docker tests completed ==="

docker-all: docker-test
	@echo "=== All Docker tests completed ==="

docker-clean:
	@echo "=== Cleaning Docker resources ==="
	-docker rmi async-test-runner 2>/dev/null || true
	-docker system prune -f 2>/dev/null || true
	@echo "Docker cleanup completed"

# ================================
# DOCKER-COMPOSE ТЕСТЫ
# ================================
compose-up:
	@echo "=== Starting docker-compose services ==="
	docker-compose up --build

compose-unit:
	@echo "=== Running unit tests via docker-compose ==="
	docker-compose run --rm test make unit-test

compose-functional:
	@echo "=== Running functional tests via docker-compose ==="
	docker-compose run --rm test make functional-test

compose-all:
	@echo "=== Running all tests via docker-compose ==="
	docker-compose run --rm test make test

compose-clean:
	@echo "=== Cleaning docker-compose resources ==="
	docker-compose down -v --rmi local
	@echo "Docker-compose cleanup completed"

# ================================
# УТИЛИТЫ И ВСПОМОГАТЕЛЬНЫЕ ЦЕЛИ
# ================================
create-service-file:
	@echo "=== Creating systemd service file ==="
	@mkdir -p systemd
	@echo '[Unit]' > systemd/async-tcp-udp-server.service
	@echo 'Description=Async TCP/UDP Server' >> systemd/async-tcp-udp-server.service
	@echo 'After=network.target' >> systemd/async-tcp-udp-server.service
	@echo '' >> systemd/async-tcp-udp-server.service
	@echo '[Service]' >> systemd/async-tcp-udp-server.service
	@echo 'Type=simple' >> systemd/async-tcp-udp-server.service
	@echo 'User=async-server' >> systemd/async-tcp-udp-server.service
	@echo 'Group=async-server' >> systemd/async-tcp-udp-server.service
	@echo 'ExecStart=/usr/local/bin/async_tcp_udp_server 8080' >> systemd/async-tcp-udp-server.service
	@echo 'Restart=always' >> systemd/async-tcp-udp-server.service
	@echo 'RestartSec=5' >> systemd/async-tcp-udp-server.service
	@echo '' >> systemd/async-tcp-udp-server.service
	@echo '[Install]' >> systemd/async-tcp-udp-server.service
	@echo 'WantedBy=multi-user.target' >> systemd/async-tcp-udp-server.service
	@echo "Service file created: systemd/async-tcp-udp-server.service"

check-systemd:
	@echo "=== Checking systemd configuration ==="
	@echo "1. Service file:"
	@if [ -f /lib/systemd/system/async-server.service ]; then \
		cat /lib/systemd/system/async-server.service; \
	else \
		echo "Service file not found at /lib/systemd/system/async-server.service"; \
	fi
	@echo ""
	@echo "2. User async-server:"
	@id async-server 2>/dev/null || echo "User 'async-server' does not exist"
	@echo ""
	@echo "3. Server binary:"
	@ls -la /usr/local/bin/async_tcp_udp_server 2>/dev/null || echo "Binary not found at /usr/local/bin/async_tcp_udp_server"
	@echo "=== Systemd check completed ==="

create-test-script:
	@echo "=== Creating test script ==="
	@echo '#!/bin/bash' > run-docker-tests.sh
	@echo 'set -e' >> run-docker-tests.sh
	@echo '' >> run-docker-tests.sh
	@echo 'echo "=== 1. Building Docker image ==="' >> run-docker-tests.sh
	@echo 'docker build -t async-test-runner .' >> run-docker-tests.sh
	@echo '' >> run-docker-tests.sh
	@echo 'echo -e "\n=== 2. Running unit tests ==="' >> run-docker-tests.sh
	@echo 'docker run --rm -v $$(pwd):/workspace -w /workspace async-test-runner make unit-test' >> run-docker-tests.sh
	@echo '' >> run-docker-tests.sh
	@echo 'echo -e "\n=== 3. Running functional tests ==="' >> run-docker-tests.sh
	@echo 'docker run --rm -v $$(pwd):/workspace -w /workspace --network host async-test-runner \' >> run-docker-tests.sh
	@echo '  bash -c "cd /workspace && /usr/bin/async_tcp_udp_server 8080 & SERVER_PID=$$! && sleep 2 && make functional-test && kill $$SERVER_PID 2>/dev/null || true"' >> run-docker-tests.sh
	@echo '' >> run-docker-tests.sh
	@echo 'echo -e "\n=== ALL TESTS COMPLETED SUCCESSFULLY! ==="' >> run-docker-tests.sh
	@chmod +x run-docker-tests.sh
	@echo "Script created: run-docker-tests.sh"

help:
	@echo "=== Available commands ==="
	@echo ""
	@echo "BUILD:"
	@echo "  all                 - Build server and client"
	@echo "  clean               - Clean build directory"
	@echo ""
	@echo "INSTALLATION:"
	@echo "  install             - Install binaries and configs"
	@echo "  uninstall           - Uninstall everything"
	@echo "  systemd-install     - Install systemd service"
	@echo "  systemd-uninstall   - Remove systemd service"
	@echo ""
	@echo "LOCAL TESTS:"
	@echo "  test                - Run unit + functional tests"
	@echo "  unit-test           - Run unit tests only"
	@echo "  functional-test     - Run functional tests"
	@echo "  systemd-test        - Run systemd tests (requires sudo)"
	@echo "  all-tests           - Run all tests"
	@echo "  clean-test          - Clean test build directory"
	@echo "  clean-all           - Clean everything"
	@echo ""
	@echo "DOCKER TESTS:"
	@echo "  docker-build        - Build Docker test image"
	@echo "  docker-test         - Run all tests in Docker"
	@echo "  docker-unit         - Run unit tests in Docker"
	@echo "  docker-functional   - Run functional tests in Docker"
	@echo "  docker-all          - Alias for docker-test"
	@echo "  docker-clean        - Clean Docker resources"
	@echo ""
	@echo "DOCKER-COMPOSE TESTS:"
	@echo "  compose-up          - Start all services with docker-compose"
	@echo "  compose-unit        - Run unit tests via docker-compose"
	@echo "  compose-functional  - Run functional tests via docker-compose"
	@echo "  compose-all         - Run all tests via docker-compose"
	@echo "  compose-clean       - Clean docker-compose resources"
	@echo ""
	@echo "UTILITIES:"
	@echo "  create-service-file - Create systemd service file template"
	@echo "  check-systemd       - Check systemd configuration"
	@echo "  create-test-script  - Create automated test script"
	@echo "  help                - Show this help"