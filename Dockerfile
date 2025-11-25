FROM ubuntu:22.04

# Установка системных зависимостей
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y \
    build-essential \
    cmake \
    g++ \
    make \
    libgtest-dev \
    libgmock-dev \
    git \
    netcat \
    net-tools \
    iputils-ping \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# Установка Google Test
RUN cd /usr/src/gtest && \
    cmake CMakeLists.txt && \
    make && \
    find . -name '*.a' -exec cp {} /usr/lib \; && \
    rm -rf /usr/src/gtest

# Создание рабочей директории
WORKDIR /workspace

# Копируем исходный код
COPY . /workspace

# Собираем проект включая тесты
RUN make clean && make all && make unit_tests && make build/functional_tests

# Создаем точку входа по умолчанию
CMD ["/bin/bash"]