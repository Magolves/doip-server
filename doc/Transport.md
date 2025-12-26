# Transport Abstraction Layer

## Overview

The transport abstraction layer decouples the DoIP protocol state machine from the underlying communication mechanism (TCP sockets). This design enables:

1. **Better testability** - State machines can be tested with mocked transports
2. **Flexibility** - Easy to add alternative transports (TLS, WebSocket, etc.)
3. **SOLID principles** - Dependency injection and interface segregation

## Architecture

### Class Diagram

```text
┌─────────────────┐
│   ITransport    │ (Interface)
│  <<abstract>>   │
├─────────────────┤
│ + sendMessage() │
│ + receiveMessage()│
│ + close()       │
│ + isActive()    │
│ + getIdentifier()│
└─────────────────┘
         △
         │ implements
    ┌────┴────┐
    │         │
┌───▼────┐  ┌─▼────────┐
│TcpTransport│  │MockTransport│
│         │  │          │
│ - socket│  │ - queues │
│ - buffer│  │          │
└─────────┘  └──────────┘
```

### Components

#### `ITransport` Interface

Pure virtual interface defining the contract for DoIP transports:

```cpp
class ITransport {
public:
    virtual ~ITransport() = default;

    // Send a DoIP message
    virtual ssize_t sendMessage(const DoIPMessage &msg) = 0;

    // Receive a DoIP message (blocking or non-blocking)
    virtual std::optional<DoIPMessage> receiveMessage() = 0;

    // Close the transport
    virtual void close() = 0;

    // Check if transport is active
    virtual bool isActive() const = 0;

    // Get transport identifier (for logging)
    virtual std::string getIdentifier() const = 0;
};
```

#### `TcpTransport` Implementation

Production implementation wrapping TCP sockets:

**Features:**
- RAII socket management (closes on destruction)
- Automatic peer identification (IP:port)
- Complete DoIP message framing (header + payload)
- Thread-safe close via atomic flag
- Detailed logging with spdlog

**Usage:**
```cpp
int socket = accept(serverSocket, ...);
auto transport = std::make_unique<TcpTransport>(socket);

// Send message
DoIPMessage msg(...);
transport->sendMessage(msg);

// Receive message
auto received = transport->receiveMessage();
if (received.has_value()) {
    handleMessage(*received);
}
```

**Implementation Details:**
- `receiveExactly()` - Handles partial TCP reads
- Buffer size: `DOIP_MAXIMUM_MTU` (configurable)
- Graceful handling of `EINTR` and `EAGAIN`
- Connection close detection (recv returns 0)

#### `MockTransport` Implementation

Test implementation using in-memory queues:

**Features:**
- Bidirectional message queues (`ThreadSafeQueue`)
- Message injection for testing incoming messages
- Message inspection for verifying sent messages
- Blocking/non-blocking modes
- Queue clearing for test isolation

**Usage:**
```cpp
// Create mock transport
auto mock = std::make_unique<MockTransport>("test-client");

// Inject incoming message (simulates server → client)
DoIPMessage request(DoIPPayloadType::RoutingActivationRequest, ...);
mock->injectMessage(request);

// Let state machine receive it
auto received = mock->receiveMessage();

// State machine processes and sends response
DoIPMessage response = statemachine.process(*received);
mock->sendMessage(response);

// Verify what was sent
auto sent = mock->popSentMessage();
REQUIRE(sent.has_value());
CHECK(sent->getPayloadType() == DoIPPayloadType::RoutingActivationResponse);
```

**Testing APIs:**
- `injectMessage(msg)` - Inject message into receive queue
- `popSentMessage()` - Get next sent message
- `hasSentMessages()` - Check if any messages were sent
- `sentMessageCount()` - Get sent queue size
- `clearQueues()` - Reset both queues
- `setBlocking(bool)` - Control blocking behavior

## Integration with DoIPDefaultConnection

### Current State (Before Refactoring)

`DoIPConnection` directly owns the TCP socket and implements both:
1. Transport logic (send/receive bytes)
2. Protocol logic (state machine)

### Target State (After Refactoring)

`DoIPDefaultConnection` accepts an `ITransport` via dependency injection:

```cpp
class DoIPDefaultConnection {
public:
    DoIPDefaultConnection(
        UniqueServerModelPtr model,
        SharedTimerManagerPtr<ConnectionTimers> timerManager,
        std::unique_ptr<ITransport> transport  // NEW: injected dependency
    );

    // Protocol logic only - no TCP details
    void handleMessage(const DoIPMessage &msg);
    void processStateTransition(DoIPServerEvent event);

private:
    std::unique_ptr<ITransport> m_transport;  // NEW: owned transport
    // State machine data...
};
```

### Migration Steps

1. **Add transport parameter** to `DoIPDefaultConnection` constructor
2. **Replace direct socket calls** with transport method calls:
   - `send(socket, ...)` → `m_transport->sendMessage(...)`
   - `recv(socket, ...)` → `m_transport->receiveMessage()`
   - `close(socket)` → `m_transport->close()`
3. **Update DoIPConnection** to create `TcpTransport`:
   ```cpp
   DoIPConnection::DoIPConnection(int socket, ...)
       : DoIPDefaultConnection(
           std::move(model),
           timerManager,
           std::make_unique<TcpTransport>(socket)  // Inject TCP transport
         )
   {
   }
   ```
4. **Update tests** to use `MockTransport` instead of real sockets

## Benefits

### 1. Testability

**Before:**
```cpp
// Hard to test - requires real TCP sockets
TEST_CASE("State machine routing activation") {
    int sockets[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
    DoIPConnection conn(sockets[0], model, timers);

    // Complex setup, hard to control timing
    write(sockets[1], requestData, size);
    conn.receiveMessage();

    // Hard to verify what was sent
    uint8_t response[1024];
    read(sockets[1], response, sizeof(response));
    // Manual parsing...
}
```

**After:**
```cpp
// Easy to test - mock transport
TEST_CASE("State machine routing activation") {
    auto mock = std::make_unique<MockTransport>();
    DoIPDefaultConnection conn(model, timers, std::move(mock));

    // Inject request
    mock->injectMessage(routingActivationRequest);

    // Process
    conn.receiveMessage();

    // Verify response
    auto response = mock->popSentMessage();
    REQUIRE(response.has_value());
    CHECK(response->getPayloadType() == DoIPPayloadType::RoutingActivationResponse);
}
```

### 2. Extensibility

Easy to add new transports:
- **TLS/SSL**: Secure DoIP communication
- **Unix Domain Sockets**: Local IPC testing
- **WebSocket**: Browser-based diagnostics
- **CAN ISO-TP**: Direct CAN bus integration

```cpp
class TlsTransport : public ITransport {
    SSL *m_ssl;
    // OpenSSL implementation...
};

// Usage unchanged
auto transport = std::make_unique<TlsTransport>(ssl);
DoIPDefaultConnection conn(model, timers, std::move(transport));
```

### 3. Separation of Concerns

**Protocol logic** (state machine, timing, validation):
- Lives in `DoIPDefaultConnection`
- No knowledge of sockets, file descriptors, or I/O

**Transport logic** (byte I/O, framing, buffering):
- Lives in `ITransport` implementations
- No knowledge of DoIP protocol semantics

## Thread Safety

- **TcpTransport**: Single-threaded use (one thread per connection)
- **MockTransport**: Thread-safe queues (`ThreadSafeQueue`) allow multi-threaded testing
- **ITransport**: Interface does not mandate thread safety (implementation-specific)

## Performance Considerations

- **Zero-copy**: `DoIPMessage::data()` returns pointer to internal buffer
- **Buffer reuse**: `TcpTransport` reuses `m_receiveBuffer` for all receives
- **Move semantics**: `MockTransport` uses `std::move()` for queue operations
- **No virtual call overhead in hot path**: Message send/receive are already abstracted

## Error Handling

All transport implementations follow consistent error semantics:

- **sendMessage()**: Returns `-1` on error, bytes sent otherwise
- **receiveMessage()**: Returns `std::nullopt` on error/close, `DoIPMessage` otherwise
- **isActive()**: Returns `false` after close or fatal error
- **close()**: Idempotent, safe to call multiple times

Callers should check `isActive()` before retrying after errors.

## Examples

### Example 1: Testing State Machine Timeout

```cpp
TEST_CASE("Routing activation timeout") {
    auto mock = std::make_unique<MockTransport>();
    auto *mockPtr = mock.get();  // Keep pointer for injection

    DoIPDefaultConnection conn(model, timers, std::move(mock));

    // Inject request
    mockPtr->injectMessage(routingActivationRequest);
    conn.receiveMessage();

    // Don't respond - let timeout occur
    std::this_thread::sleep_for(std::chrono::seconds(6));

    // Verify timeout handling
    CHECK_FALSE(conn.isActive());
    auto nack = mockPtr->popSentMessage();
    CHECK(nack->getPayloadType() == DoIPPayloadType::GenericHeaderNegativeAcknowledge);
}
```

### Example 2: Testing Invalid Message Handling

```cpp
TEST_CASE("Invalid message causes disconnect") {
    auto mock = std::make_unique<MockTransport>();
    auto *mockPtr = mock.get();

    DoIPDefaultConnection conn(model, timers, std::move(mock));

    // Inject malformed message
    uint8_t badPayload[] = {0xFF, 0xFF};
    DoIPMessage invalid(static_cast<DoIPPayloadType>(0xFFFF), badPayload, 2);
    mockPtr->injectMessage(invalid);

    conn.receiveMessage();

    // Should close connection
    CHECK_FALSE(mockPtr->isActive());
}
```

### Example 3: Production TCP Usage

```cpp
void DoIPServer::acceptConnection() {
    int clientSocket = accept(m_serverSocket, ...);

    // Create TCP transport
    auto transport = std::make_unique<TcpTransport>(clientSocket);

    // Create connection with injected transport
    auto connection = std::make_unique<DoIPConnection>(
        std::move(model),
        timerManager,
        std::move(transport)
    );

    m_connections.push_back(std::move(connection));
}
```

## Future Enhancements

1. **Async I/O**: Non-blocking transport with callbacks
2. **Transport statistics**: Bytes sent/received, error counts
3. **Transport configuration**: Buffer sizes, timeout settings
4. **Connection pooling**: Reusable transport objects
5. **Serialization**: Custom transport for inter-process communication

## Related Files

- `inc/ITransport.h` - Interface definition
- `inc/TcpTransport.h` / `src/TcpTransport.cpp` - TCP implementation
- `inc/MockTransport.h` / `src/MockTransport.cpp` - Mock implementation
- `test/unit/MockTransport_Test.cpp` - Unit tests
- `inc/ThreadSafeQueue.h` - Queue used by MockTransport

## References

- **Dependency Injection**: [Martin Fowler's article](https://martinfowler.com/articles/injection.html)
- **SOLID Principles**: Interface Segregation, Dependency Inversion
- **Strategy Pattern**: Transport selection at runtime
