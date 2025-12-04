#!/bin/bash

set -e

echo "=== Running All Tests ==="

echo "1. Running unit tests..."
make unit-test

echo "2. Running functional tests..."
make functional-test

echo "3. To run systemd tests: sudo make systemd-test"
echo "   or: sudo tests/functional/systemd_test.sh"

echo "=== All Tests Complete ==="