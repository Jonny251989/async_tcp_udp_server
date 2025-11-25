#!/bin/bash

# Этот скрипт теперь не используется в основных тестах,
# но оставлен для ручного тестирования

TEST_NAME=$1
INPUT_FILE="test_data/${TEST_NAME}_input.txt"
OUTPUT_FILE="test_data/${TEST_NAME}_output.txt"

# Получаем абсолютный путь к директории скрипта
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

INPUT_FILE="$PROJECT_ROOT/tests/test_data/${TEST_NAME}_input.txt"
OUTPUT_FILE="$PROJECT_ROOT/tests/test_data/${TEST_NAME}_output.txt"

# Создаем выходной файл
touch "$OUTPUT_FILE"

# Определяем, какой клиент использовать: TCP или UDP
if [[ $TEST_NAME == *"udp"* ]]; then
    CLIENT="$PROJECT_ROOT/build/udp_client"
else
    CLIENT="$PROJECT_ROOT/build/tcp_client"
fi

# Читаем входной файл, который содержит одну строку - сообщение для отправки
if [ ! -f "$INPUT_FILE" ]; then
    echo "ERROR: Input file not found: $INPUT_FILE" > "$OUTPUT_FILE"
    exit 1
fi

MESSAGE=$(cat "$INPUT_FILE")

# Запускаем клиент с сообщением и сохраняем вывод
echo "Running: $CLIENT 127.0.0.1 8080 \"$MESSAGE\"" > "$OUTPUT_FILE"
$CLIENT 127.0.0.1 8080 "$MESSAGE" >> "$OUTPUT_FILE" 2>&1
EXIT_CODE=$?

# Если клиент завершился с ошибкой, добавляем информацию в выходной файл
if [ $EXIT_CODE -ne 0 ]; then
    echo "Client exited with code: $EXIT_CODE" >> "$OUTPUT_FILE"
fi

exit $EXIT_CODE