#!/bin/bash

echo "=== Full Server Test ==="

# Test TCP
echo "1. Testing TCP mirror:"
echo -e "Hello World\nn" | async_client tcp 127.0.0.1 8080

echo "2. Testing TCP /time:"
echo -e "/time\nn" | async_client tcp 127.0.0.1 8080

echo "3. Testing TCP /stats:"
echo -e "/stats\nn" | async_client tcp 127.0.0.1 8080

# Test UDP
echo "4. Testing UDP mirror:"
echo -e "UDP Test\nn" | async_client udp 127.0.0.1 8080

echo "5. Testing UDP /time:"
echo -e "/time\nn" | async_client udp 127.0.0.1 8080

echo "=== Test Complete ==="