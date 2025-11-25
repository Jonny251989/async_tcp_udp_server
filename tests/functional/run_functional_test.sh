#!/bin/bash

TEST_NAME=$1
INPUT_FILE="/test_data/${TEST_NAME}_input.txt"
OUTPUT_FILE="/test_data/${TEST_NAME}_output.txt"

# Получаем абсолютный путь к директории скрипта
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Альтернативные пути для локального запуска
LOCAL_INPUT_FILE="$PROJECT_ROOT/tests/test_data/${TEST_NAME}_input.txt"
LOCAL_OUTPUT_FILE="$PROJECT_ROOT/tests/test_data/${TEST_NAME}_output.txt"

# Создаем выходной файл
touch "$OUTPUT_FILE" 2>/dev/null || touch "$LOCAL_OUTPUT_FILE"

# Определяем, какой клиент использовать: TCP или UDP
if [[ $TEST_NAME == *"udp"* ]]; then
    CLIENT="/app/udp_client"
    # Альтернативный путь для локального запуска
    LOCAL_CLIENT="$PROJECT_ROOT/build/udp_client"
else
    CLIENT="/app/tcp_client"
    # Альтернативный путь для локального запуска
    LOCAL_CLIENT="$PROJECT_ROOT/build/tcp_client"
fi

# Проверяем, какой клиент доступен
if [ -f "$CLIENT" ]; then
    CLIENT_TO_USE="$CLIENT"
    INPUT_TO_USE="$INPUT_FILE"
    OUTPUT_TO_USE="$OUTPUT_FILE"
elif [ -f "$LOCAL_CLIENT" ]; then
    CLIENT_TO_USE="$LOCAL_CLIENT"
    INPUT_TO_USE="$LOCAL_INPUT_FILE"
    OUTPUT_TO_USE="$LOCAL_OUTPUT_FILE"
else
    echo "ERROR: No client found" > "$OUTPUT_TO_USE" 2>/dev/null || echo "ERROR: No client found"
    exit 1
fi

# Читаем входной файл, который содержит одну строку - сообщение для отправки
if [ ! -f "$INPUT_TO_USE" ]; then
    echo "ERROR: Input file not found: $INPUT_TO_USE" > "$OUTPUT_TO_USE"
    exit 1
fi

MESSAGE=$(cat "$INPUT_TO_USE")

# Определяем хост сервера (для Docker используем имя сервиса, для локально - localhost)
if [ -f "$CLIENT" ]; then
    SERVER_HOST="telemetry_server"
else
    SERVER_HOST="127.0.0.1"
fi

# Запускаем клиент с сообщением и сохраняем вывод
echo "Running: $CLIENT_TO_USE $SERVER_HOST 8080 \"$MESSAGE\"" > "$OUTPUT_TO_USE"
$CLIENT_TO_USE $SERVER_HOST 8080 "$MESSAGE" >> "$OUTPUT_TO_USE" 2>&1
EXIT_CODE=$?

# Если клиент завершился с ошибкой, добавляем информацию в выходной файл
if [ $EXIT_CODE -ne 0 ]; then
    echo "Client exited with code: $EXIT_CODE" >> "$OUTPUT_TO_USE"
fi

exit $EXIT_CODE