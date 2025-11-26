CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -pthread -O2 -Icommon -Iserver
LDFLAGS = -pthread

# Директории
COMMON_DIR = common
SERVER_DIR = server
CLIENT_DIR = client
BUILD_DIR = build

# Исходные файлы
COMMON_SOURCES = $(COMMON_DIR)/session_manager.cpp
SERVER_SOURCES = $(SERVER_DIR)/main.cpp $(SERVER_DIR)/server.cpp \
                 $(SERVER_DIR)/tcp_handler.cpp $(SERVER_DIR)/udp_handler.cpp \
                 $(SERVER_DIR)/tcp_connection.cpp $(SERVER_DIR)/command_processor.cpp \
                 $(SERVER_DIR)/eventloop.cpp $(SERVER_DIR)/command.cpp
CLIENT_SOURCES = $(CLIENT_DIR)/tcp_client.cpp $(CLIENT_DIR)/udp_client.cpp

# Объектные файлы
COMMON_OBJS = $(patsubst $(COMMON_DIR)/%.cpp,$(BUILD_DIR)/common/%.o,$(COMMON_SOURCES))
SERVER_OBJS = $(patsubst $(SERVER_DIR)/%.cpp,$(BUILD_DIR)/server/%.o,$(SERVER_SOURCES))
CLIENT_OBJS = $(patsubst $(CLIENT_DIR)/%.cpp,$(BUILD_DIR)/client/%.o,$(CLIENT_SOURCES))

TARGET_SERVER = $(BUILD_DIR)/async_tcp_udp_server
TARGET_TCP_CLIENT = $(BUILD_DIR)/tcp_client
TARGET_UDP_CLIENT = $(BUILD_DIR)/udp_client

.PHONY: all clean quick_test stop_server

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

clean:
	rm -rf $(BUILD_DIR)

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

# Остановка сервера
stop_server:
	@echo "Stopping running servers..."
	@-pkill -f "async_tcp_udp_server" 2>/dev/null || true
	@-sudo kill -9 $(shell sudo lsof -ti:8080) 2>/dev/null || true
	@-sudo kill -9 $(shell sudo lsof -ti:8081) 2>/dev/null || true
	@echo "Servers stopped"

# Установка (опционально)
install: $(TARGET_SERVER)
	install -m 755 $(TARGET_SERVER) /usr/local/bin/async_tcp_udp_server

# Удаление (опционально)
uninstall:
	rm -f /usr/local/bin/async_tcp_udp_server

# Отладочная сборка
debug: CXXFLAGS += -g -DDEBUG
debug: clean all

# Профилировочная сборка
profile: CXXFLAGS += -pg
profile: LDFLAGS += -pg
profile: clean all

# Помощь
help:
	@echo "Доступные цели:"
	@echo "  all          - сборка всех целей (по умолчанию)"
	@echo "  clean        - очистка сборки"
	@echo "  quick_test   - быстрый тест сервера и клиентов"
	@echo "  stop_server  - остановка всех запущенных серверов"
	@echo "  debug        - отладочная сборка"
	@echo "  profile      - профилировочная сборка"
	@echo "  install      - установка сервера в систему"
	@echo "  uninstall    - удаление сервера из системы"
	@echo "  help         - эта справка"