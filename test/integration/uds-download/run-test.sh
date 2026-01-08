#!/bin/bash

# Start server in background, log to server.log
echo "Check for running servers..." > server.log 2>&1
ps -aux | grep Server >> server.log 2>&1
echo "Starting UdsServer..." >> server.log 2>&1
./UdsServer >> server.log 2>&1 &
SERVER_PID=$!

# Give the server a moment to start
sleep 1

# Run client, log to client.log
ls -la > client.log 2>&1
python3 test-uds-download.py >> client.log 2>&1
CLIENT_EXIT=$?

# Kill server
kill $SERVER_PID

# Print logs for debugging
echo "=== Server Log ==="
tail server.log
echo "=== Client Log ==="
cat client.log

# Return client's exit code
exit $CLIENT_EXIT