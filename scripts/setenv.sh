#!/bin/bash

# Script to set environment variables for the server

export SERVER_PORT=${1:-8080}
export LOG_LEVEL=${2:-info}
export MAX_CONNECTIONS=${3:-1000}

echo "Environment variables set:"
echo "  SERVER_PORT=$SERVER_PORT"
echo "  LOG_LEVEL=$LOG_LEVEL"
echo "  MAX_CONNECTIONS=$MAX_CONNECTIONS"
echo ""
echo "Now you can run: ./build/async_tcp_udp_server"