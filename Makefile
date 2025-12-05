.PHONY: all clean install uninstall systemd-install systemd-uninstall test unit-test functional-test systemd-test all-tests clean-all \
        docker-build docker-unit-test docker-functional-test docker-test docker-clean \
        compose-up compose-down compose-unit-test compose-functional-test compose-all compose-clean compose-systemd-test \
        service-file check-systemd test-script help

# ===== КОНФИГУРАЦИЯ =====
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -pthread -O2 -I. -Icommon -Iserver -Iclient
LDFLAGS = -pthread

BUILD_DIR = build

# Файлы
SERVER_SRCS = server/main.cpp server/server.cpp server/tcp_handler.cpp server/udp_handler.cpp \
              server/tcp_connection.cpp server/command_processor.cpp server/eventloop.cpp \
              server/command.cpp server/session_manager.cpp
SERVER_OBJS = $(SERVER_SRCS:%.cpp=$(BUILD_DIR)/%.o)
CLIENT_OBJS = $(BUILD_DIR)/client/main.o

# ===== СБОРКА =====
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

# ===== УСТАНОВКА =====
install: all
	install -d /usr/local/bin /etc/async-tcp-udp-server
	install -m755 $(BUILD_DIR)/async_tcp_udp_server /usr/local/bin/
	install -m755 $(BUILD_DIR)/client_app /usr/local/bin/async_client
	install -m644 config/server.conf.example /etc/async-tcp-udp-server/server.conf 2>/dev/null || true

uninstall:
	@systemctl stop async-server.service 2>/dev/null || true
	@systemctl disable async-server.service 2>/dev/null || true
	@rm -f /lib/systemd/system/async-server.service /usr/local/bin/async_tcp_udp_server /usr/local/bin/async_client
	@rm -rf /etc/async-tcp-udp-server
	@systemctl daemon-reload 2>/dev/null || true

systemd-install: install
	@install -d /lib/systemd/system
	@if [ -f systemd/async-tcp-udp-server.service ]; then \
		install -m644 systemd/async-tcp-udp-server.service /lib/systemd/system/async-server.service; \
	else \
		printf '[Unit]\nDescription=Async TCP/UDP Server\nAfter=network.target\n\n[Service]\nType=simple\nUser=async-server\nGroup=async-server\nExecStart=/usr/local/bin/async_tcp_udp_server 8080\nRestart=on-failure\nRestartSec=5\nStandardOutput=journal\n\n[Install]\nWantedBy=multi-user.target\n' > /lib/systemd/system/async-server.service; \
	fi
	@id async-server >/dev/null 2>&1 || useradd -r -s /usr/sbin/nologin async-server 2>/dev/null || true
	@systemctl daemon-reload && systemctl enable async-server.service

# ===== ТЕСТЫ =====
unit-test: $(BUILD_DIR)/tests/unit_tests
	@$(BUILD_DIR)/tests/unit_tests

functional-test: $(BUILD_DIR)/async_tcp_udp_server
	@PORT=8081 bash tests/functional/test_server.sh

systemd-test: all
	@sudo bash tests/functional/systemd_test.sh

test: unit-test functional-test

all-tests: test

$(BUILD_DIR)/tests/unit_tests: $(patsubst %.cpp,$(BUILD_DIR)/%.o,tests/unit/test_main.cpp tests/unit/test_command_processor.cpp tests/unit/test_session_manager.cpp) \
                               $(BUILD_DIR)/server/command.o $(BUILD_DIR)/server/session_manager.o $(BUILD_DIR)/server/command_processor.o
	@mkdir -p $(BUILD_DIR)/tests
	$(CXX) $(CXXFLAGS) $^ -lgtest -lgtest_main -lpthread -o $@

clean-all: clean

# ===== DOCKER =====
docker-build:
	docker build -t async-test-runner .

docker-unit-test: docker-build
	docker run --rm -v "$(PWD)":/workspace -w /workspace async-test-runner make unit-test

docker-functional-test: docker-build
	docker run --rm -v "$(PWD)":/workspace -w /workspace --network host async-test-runner make functional-test

docker-test: docker-build
	docker run --rm -v "$(PWD)":/workspace -w /workspace async-test-runner make test

docker-clean:
	-docker rmi async-test-runner 2>/dev/null || true

# ===== DOCKER-COMPOSE =====
compose-up:
	docker-compose up --build

compose-down:
	docker-compose down

compose-unit-test:
	docker-compose run --rm test make unit-test

compose-functional-test:
	docker-compose run --rm test make functional-test

compose-all:
	docker-compose run --rm test make test

compose-systemd-test:
	docker-compose run --rm systemd-test

compose-clean:
	docker-compose down -v --remove-orphans

# ===== УТИЛИТЫ =====
service-file:
	@mkdir -p systemd
	@printf '[Unit]\nDescription=Async TCP/UDP Server\nAfter=network.target\n\n[Service]\nType=simple\nUser=async-server\nGroup=async-server\nExecStart=/usr/local/bin/async_tcp_udp_server 8080\nRestart=on-failure\nRestartSec=5\nStandardOutput=journal\n\n[Install]\nWantedBy=multi-user.target\n' > systemd/async-tcp-udp-server.service

check-systemd:
	@echo "Service file:"; [ -f /lib/systemd/system/async-server.service ] && cat /lib/systemd/system/async-server.service || echo "Not found"
	@echo -e "\nUser:"; id async-server 2>/dev/null || echo "Not found"
	@echo -e "\nBinary:"; [ -f /usr/local/bin/async_tcp_udp_server ] && ls -la /usr/local/bin/async_tcp_udp_server || echo "Not found"

test-script:
	@echo '#!/bin/bash\nset -e\ndocker build -t async-test-runner .\ndocker run --rm -v $$(pwd):/workspace -w /workspace async-test-runner make unit-test\ndocker run --rm -v $$(pwd):/workspace -w /workspace --network host async-test-runner make functional-test\necho "All tests passed!"' > run-docker-tests.sh
	@chmod +x run-docker-tests.sh

help:
	@echo "BUILD:          all clean"
	@echo "INSTALL:        install uninstall systemd-install"
	@echo "LOCAL TESTS:    test unit-test functional-test systemd-test all-tests clean-all"
	@echo "DOCKER TESTS:   docker-build docker-test docker-unit-test docker-functional-test docker-clean"
	@echo "COMPOSE TESTS:  compose-up compose-down compose-unit-test compose-functional-test compose-all compose-systemd-test compose-clean"
	@echo "UTILITIES:      service-file check-systemd test-script help"