# Проект: Асинхронная клиент-серверная модель
# Основные требования
        ОС: Linux (Ubuntu 22.04+, Debian 11+, CentOS 8+), Windows/WSL2, macOS

        Компилятор: g++ ≥10.0 (поддержка C++20)

        Инструменты: make ≥4.0, CMake ≥3.16

        Библиотеки: pthread, systemd, Google Test (для тестов)

# Команды для сборки
        make all            собрать всё
        make clean          очистить сборку
        make test           запустить тесты
        make docker-build   собрать образ
        make docker-test    запустить тесты в контейнере

# dev пакет
        async-tcp-udp-server_1.0.0_amd64.deb

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

# КОМАНДЫ:

    /time             - Получить время сервера
    /stats            - Статистика подключений
    /shutdown         - Завершить работу сервера

# II. Запуск тестов для автоматической проверки работы клиент-серверной модели

    Запуск unit tests:
        make unit-test

    Запуск functional tests:
        make functional-test

    Запуск systemd tests: 
        sudo make systemd-test
        sudo tests/functional/systemd_test.sh



# III. Запуск клиент-серверной модели через Docker


        docker build -t async-systemd . 

        docker run -d \
        --name async-test \
        --privileged \
        --tmpfs /run \
        --tmpfs /tmp \
        -p 8080:8080 \
        -p 8080:8080/udp \
        async-systemd-new

        sleep 5

        echo "1. /time: "; echo "/time" | nc -N localhost 8080
        echo "2. /stats: "; echo "/stats" | nc -N localhost 8080
        echo "3. Mirror test: "; echo "Hello World" | nc -N localhost 8080
        echo "4. Unknown command: "; echo "/unknown" | nc -N localhost 8080


# IV. Запуск клиент-серверной модели c помощью docker-compose

        docker-compose down --remove-orphans

        docker build -t async-test-runner .

        docker-compose run --rm test
        docker-compose run --rm systemd-test

        docker-compose down