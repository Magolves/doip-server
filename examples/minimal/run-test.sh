#!/bin/bash
set -e
./MinimalDoIPServer &
SERVER_PID=$!
sleep 1

# Run tests
./MinimalDoIPClient

kill $SERVER_PID