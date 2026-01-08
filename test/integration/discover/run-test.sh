#!/bin/bash
set -e
./DiscoverServer &
SERVER_PID=$!
sleep 1

# Run tests
./DiscoverClient
CLIENT_EXIT=$?

kill $SERVER_PID

# Return client's exit code
exit $CLIENT_EXIT