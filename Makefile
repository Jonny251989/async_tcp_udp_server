CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread -O2 -Icommon -Iserver
LDFLAGS = -pthread
GTEST_FLAGS = -lgtest -lgtest_main -pthread

# Directories
COMMON_DIR = common
SERVER_DIR = server
CLIENT_DIR = client
BUILD_DIR = build
TESTS_DIR = tests
UNIT_TESTS_DIR = $(TESTS_DIR)/unit
FUNCTIONAL_TESTS_DIR = $(TESTS_DIR)/functional
TEST_DATA_DIR = $(TESTS_DIR)/test_data

# Source files
COMMON_SOURCES = $(COMMON_DIR)/protocol.cpp $(COMMON_DIR)/network_utils.cpp
SERVER_SOURCES = $(SERVER_DIR)/main.cpp $(SERVER_DIR)/server.cpp \
                 $(SERVER_DIR)/tcp_handler.cpp $(SERVER_DIR)/udp_handler.cpp \
                 $(SERVER_DIR)/tcp_connection.cpp $(SERVER_DIR)/command_processor.cpp \
                 $(SERVER_DIR)/session_manager.cpp
CLIENT_SOURCES = $(CLIENT_DIR)/tcp_client.cpp $(CLIENT_DIR)/udp_client.cpp

# Object files
COMMON_OBJS = $(patsubst $(COMMON_DIR)/%.cpp,$(BUILD_DIR)/common/%.o,$(COMMON_SOURCES))
SERVER_OBJS = $(patsubst $(SERVER_DIR)/%.cpp,$(BUILD_DIR)/server/%.o,$(SERVER_SOURCES))
CLIENT_OBJS = $(patsubst $(CLIENT_DIR)/%.cpp,$(BUILD_DIR)/client/%.o,$(CLIENT_SOURCES))

TARGET_SERVER = $(BUILD_DIR)/async_tcp_udp_server
TARGET_TCP_CLIENT = $(BUILD_DIR)/tcp_client
TARGET_UDP_CLIENT = $(BUILD_DIR)/udp_client

# Test targets
UNIT_TESTS = $(BUILD_DIR)/unit_tests
FUNCTIONAL_TESTS = $(BUILD_DIR)/functional_tests

.PHONY: all clean test unit_tests functional_tests quick_test prepare_test_data
.PHONY: docker-build docker-build-service docker-run-server docker-run-tests docker-clean
.PHONY: docker-unit-tests docker-functional-tests docker-full-test docker-client

all: $(TARGET_SERVER) $(TARGET_TCP_CLIENT) $(TARGET_UDP_CLIENT)

$(TARGET_SERVER): $(COMMON_OBJS) $(SERVER_OBJS)
	@mkdir -p $(@D)
	$(CXX) $^ -o $@ $(LDFLAGS)

$(TARGET_TCP_CLIENT): $(BUILD_DIR)/client/tcp_client.o
	@mkdir -p $(@D)
	$(CXX) $^ -o $@

$(TARGET_UDP_CLIENT): $(BUILD_DIR)/client/udp_client.o
	@mkdir -p $(@D)
	$(CXX) $^ -o $@

$(BUILD_DIR)/common/%.o: $(COMMON_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/server/%.o: $(SERVER_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/client/%.o: $(CLIENT_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Unit tests
$(UNIT_TESTS): $(UNIT_TESTS_DIR)/unit_tests.cpp $(UNIT_TESTS_DIR)/main.cpp $(COMMON_OBJS) $(filter-out $(BUILD_DIR)/server/main.o, $(SERVER_OBJS))
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(GTEST_FLAGS)

# Functional tests
$(FUNCTIONAL_TESTS): $(FUNCTIONAL_TESTS_DIR)/functional_tests.cpp $(FUNCTIONAL_TESTS_DIR)/main.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(GTEST_FLAGS)

# Prepare test data
prepare_test_data:
	@mkdir -p $(TEST_DATA_DIR)
	@echo "Hello World" > $(TEST_DATA_DIR)/echo_tcp_input.txt
	@echo "/time" > $(TEST_DATA_DIR)/time_tcp_input.txt
	@echo "/stats" > $(TEST_DATA_DIR)/stats_tcp_input.txt
	@echo "Hello UDP" > $(TEST_DATA_DIR)/echo_udp_input.txt
	@echo "/time" > $(TEST_DATA_DIR)/time_udp_input.txt
	@chmod +x $(FUNCTIONAL_TESTS_DIR)/run_functional_test.sh

# Test targets
unit_tests: $(UNIT_TESTS)
	@echo "Running unit tests..."
	./$(UNIT_TESTS)

functional_tests: $(TARGET_SERVER) $(TARGET_TCP_CLIENT) $(TARGET_UDP_CLIENT) $(FUNCTIONAL_TESTS) prepare_test_data
	@echo "Running functional tests..."
	./$(FUNCTIONAL_TESTS)

test: unit_tests functional_tests

clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(TEST_DATA_DIR)

# Быстрый тест сервера
quick_test: $(TARGET_SERVER) $(TARGET_TCP_CLIENT) $(TARGET_UDP_CLIENT)
	@echo "Starting quick test..."
	@echo "Starting server in background..."
	@./$(TARGET_SERVER) 8081 &
	@SERVER_PID=$$!; \
	sleep 3; \
	echo "Testing TCP client..."; \
	./$(TARGET_TCP_CLIENT) 127.0.0.1 8081 "Hello World" || echo "TCP client test failed"; \
	echo "Testing UDP client..."; \
	./$(TARGET_UDP_CLIENT) 127.0.0.1 8081 "Hello UDP" || echo "UDP client test failed"; \
	echo "Testing commands..."; \
	./$(TARGET_TCP_CLIENT) 127.0.0.1 8081 "/time" || echo "Time command failed"; \
	./$(TARGET_TCP_CLIENT) 127.0.0.1 8081 "/stats" || echo "Stats command failed"; \
	echo "Stopping server..."; \
	if kill $$SERVER_PID 2>/dev/null; then \
		wait $$SERVER_PID 2>/dev/null; \
	else \
		echo "Server already stopped"; \
	fi
	@echo "Quick test completed"

# Docker targets

# Сборка Docker образа
docker-build:
	docker-compose --profile build up builder

# Сборка конкретного сервиса
docker-build-service:
	docker-compose build $(service)

# Запуск сервера в Docker
docker-run-server:
	docker-compose up telemetry_server

# Запуск всех тестов в Docker
docker-run-tests:
	docker-compose --profile test up unit_tests functional_tests quick_test

# Запуск только unit тестов в Docker
docker-unit-tests:
	docker-compose --profile test up unit_tests

# Запуск только functional тестов в Docker
docker-functional-tests:
	docker-compose --profile test up functional_tests

# Очистка Docker
docker-clean:
	docker-compose down -v
	docker system prune -f

# Полный цикл тестирования в Docker
docker-full-test: docker-build docker-run-tests

# Интерактивный клиент для тестирования
docker-client:
	docker-compose --profile test run --rm telemetry_client /bin/bash

# Остановка сервера
stop_server:
	@echo "Stopping running servers..."
	@-pkill -f "telemetry_server" 2>/dev/null || true
	@-sudo kill -9 $(shell sudo lsof -ti:8080) 2>/dev/null || true
	@-sudo kill -9 $(shell sudo lsof -ti:8081) 2>/dev/null || true
	@echo "Servers stopped"