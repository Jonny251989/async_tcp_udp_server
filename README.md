

# КОМАНДЫ:

    /time             - Получить время сервера
    /stats            - Статистика подключений
    /shutdown         - Завершить работу сервера

# Запуск локально через разные терминалы:

    Запустите сервер в первом терминале
        ./build/async_tcp_udp_server 8080

    В другом терминале запустите TCP клиент
        ./build/client_app tcp 127.0.0.1 8080

    Отправьте сообщения:
        Input: Hello World
        Output: Hello World (echo)

        Input: /time
        Output: 2024-01-15 14:30:25 (время на момент запроса)

        Input: /stats
        Output: Total connections: 5, Current connections: 2

        Input: /shutdown
        Output: /SHUTDOWN_ACK (сервер завершит работу)

# II. Запуск тестов для автоматической проверки работы клиент-серверной модели

    Запуск unit tests:
        make unit-test

    Запуск functional tests:
        make functional-test

    Запуск systemd tests: 
        sudo make systemd-test
        sudo tests/functional/systemd_test.sh



# III. Запуск клиент-серверной модели через Docker

    Cобрать образ:
        docker build -t async-systemd .

    Запустить контейнер:
        docker run -d \
        --name async-test \
        --privileged \
        --tmpfs /run \
        --tmpfs /tmp \
        -p 8080:8080 \
        -p 8080:8080/udp \
        async-systemd

    Проверить:
        sleep 5

        echo "1. /time: "; echo "/time" | nc -N localhost 8080
        echo "2. /stats: "; echo "/stats" | nc -N localhost 8080
        echo "3. Mirror test: "; echo "Hello World" | nc -N localhost 8080
        echo "4. Unknown command: "; echo "/unknown" | nc -N localhost 8080