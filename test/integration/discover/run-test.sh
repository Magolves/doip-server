#!/bin/bash
set -e
./DiscoverServer > server.log 2>&1 &
SERVER_PID=$!
sleep 1

# Run tests
./DiscoverClient > client.log 2>&1
CLIENT_EXIT=$?

kill $SERVER_PID

# Print logs for debugging
echo "=== Server Log ==="
tail server.log
echo "=== Client Log ==="
cat client.log


# Return client's exit code
exit $CLIENT_EXIT