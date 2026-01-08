#!/bin/bash

# Start server in background, log to server.log
./UdsServer > server.log 2>&1 &
SERVER_PID=$!

# Give the server a moment to start
sleep 1

# Run client, log to client.log
python3 ./test-local-client.py > client.log 2>&1
CLIENT_EXIT=$?

# Kill server
kill $SERVER_PID
pkill UdsServer

# Print logs for debugging
echo "=== Server Log ==="
tail server.log
echo "=== Client Log ==="
cat client.log

# Return client's exit code
exit $CLIENT_EXIT