#!/bin/bash
set -e
./MinimalDoIPServer &
SERVER_PID=$!
sleep 1

# Run tests
./MinimalDoIPClient
CLIENT_EXIT=$?

kill $SERVER_PID

sleep 1

# Return client's exit code
exit $CLIENT_EXIT