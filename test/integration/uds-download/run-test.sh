#!/bin/bash

# Wait for port 13400 to be free (max 10 seconds)
echo "Checking if port 13400 is available..."
TIMEOUT=100  # 10 seconds (100 * 0.1s)
ELAPSED=0

while [ $ELAPSED -lt $TIMEOUT ]; do
    PORT_IN_USE=0

    if command -v ss >/dev/null 2>&1; then
        # Linux: use ss
        if ss -tln 2>/dev/null | grep -q ':13400'; then
            PORT_IN_USE=1
        fi
    elif command -v netstat >/dev/null 2>&1; then
        # macOS/BSD: use netstat
        if netstat -an 2>/dev/null | grep -q '\.13400.*LISTEN'; then
            PORT_IN_USE=1
        fi
    elif command -v lsof >/dev/null 2>&1; then
        # Fallback: use lsof
        if lsof -i :13400 >/dev/null 2>&1; then
            PORT_IN_USE=1
        fi
    fi

    if [ $PORT_IN_USE -eq 0 ]; then
        echo "Port 13400 is available"
        break
    fi

    if [ $ELAPSED -eq 0 ]; then
        echo "Port 13400 is still in use, waiting..."
    fi

    sleep 0.1
    ELAPSED=$((ELAPSED + 1))
done

if [ $PORT_IN_USE -eq 1 ]; then
    echo "ERROR: Port 13400 still in use after 10 seconds timeout" >&2
    exit 1
fi

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