#!/bin/bash

set -e

echo "=== Running All Tests ==="

# 1. Unit tests
echo "1. Running unit tests..."
make unit-test

# 2. Functional tests (без systemd)
echo "2. Running functional tests..."
make functional-test

# 3. Systemd tests (требует sudo)
echo "3. To run systemd tests: sudo make systemd-test"
echo "   or: sudo tests/functional/systemd_test.sh"

echo "=== All Tests Complete ==="