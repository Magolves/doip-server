Below is a clean, layered design proposal for an ISO-14229 (UDS) library in C++17, suitable for embedded ECUs, PC tools, or HIL systems. The design emphasizes separation of concerns, testability, and transport-agnosticism (CAN, CAN-FD, DoIP).

⸻

1. Design Goals

✔ ISO-14229 compliant
✔ Transport independent (ISO-15765-2, DoIP, custom)
✔ Client & Server support
✔ Deterministic, embedded-friendly
✔ No RTTI / minimal dynamic allocation
✔ Testable (mock transports)
✔ C++17 (no coroutines)

⸻

2. Layered Architecture

+--------------------------------------------------+
| Application (ECU logic / Diagnostic tool)        |
+--------------------------------------------------+
| UDS Services (ISO 14229)                          |
|  - ReadDataByIdentifier                          |
|  - DiagnosticSessionControl                      |
|  - SecurityAccess                                |
+--------------------------------------------------+
| UDS Core                                         |
|  - Request / Response                            |
|  - Negative Response Handling                    |
|  - Service Dispatch                              |
+--------------------------------------------------+
| Transport Abstraction                            |
|  - CAN (ISO-TP / ISO 15765-2)                    |
|  - DoIP                                          |
|  - Custom                                        |
+--------------------------------------------------+
| OS / HAL / Driver                                |
+--------------------------------------------------+


⸻

3. Namespace Layout

namespace uds
{
    namespace core {}
    namespace services {}
    namespace transport {}
    namespace types {}
    namespace server {}
    namespace client {}
}


⸻

4. Core Data Types

4.1 UDS Types

namespace uds::types
{
    using Byte  = uint8_t;
    using Sid   = uint8_t;
    using Did   = uint16_t;

    enum class NRC : Byte
    {
        GeneralReject                 = 0x10,
        ServiceNotSupported           = 0x11,
        SubFunctionNotSupported       = 0x12,
        IncorrectMessageLength        = 0x13,
        ConditionsNotCorrect          = 0x22,
        SecurityAccessDenied          = 0x33,
        InvalidKey                    = 0x35,
        RequestOutOfRange             = 0x31,
        ResponsePending               = 0x78,
    };
}


⸻

5. UDS Message Model

5.1 Request / Response

namespace uds::core
{
    struct Request
    {
        types::Sid sid;
        std::vector<uint8_t> payload;
    };

    struct Response
    {
        bool positive;
        types::Sid sid;
        types::NRC nrc;
        std::vector<uint8_t> payload;
    };
}


⸻

6. Transport Abstraction

6.1 Transport Interface

namespace uds::transport
{
    class ITransport
    {
    public:
        virtual ~ITransport() = default;

        virtual bool send(const std::vector<uint8_t>& data) = 0;
        virtual bool receive(std::vector<uint8_t>& data,
                             uint32_t timeout_ms) = 0;
    };
}


⸻

6.2 CAN ISO-TP Transport Example

class CanIsoTpTransport : public uds::transport::ITransport
{
public:
    bool send(const std::vector<uint8_t>& data) override;
    bool receive(std::vector<uint8_t>& data,
                 uint32_t timeout_ms) override;
};


⸻

7. UDS Core Layer

7.1 Service Base Class

namespace uds::core
{
    class IService
    {
    public:
        virtual ~IService() = default;

        virtual types::Sid sid() const = 0;

        virtual Response handle(const Request& req) = 0;
    };
}


⸻

7.2 Service Dispatcher (Server Side)

namespace uds::server
{
    class Dispatcher
    {
    public:
        void register_service(core::IService* service);

        core::Response dispatch(const core::Request& request);

    private:
        std::unordered_map<types::Sid, core::IService*> services_;
    };
}


⸻

8. UDS Services

8.1 Example: Diagnostic Session Control (0x10)

namespace uds::services
{
    class DiagnosticSessionControl : public core::IService
    {
    public:
        types::Sid sid() const override { return 0x10; }

        core::Response handle(const core::Request& req) override;
    };
}


⸻

8.2 Example: Read Data By Identifier (0x22)

class ReadDataByIdentifier : public core::IService
{
public:
    using ReadCallback = std::function<bool(types::Did, std::vector<uint8_t>&)>;

    explicit ReadDataByIdentifier(ReadCallback cb);

    types::Sid sid() const override { return 0x22; }

    core::Response handle(const core::Request& req) override;

private:
    ReadCallback read_cb_;
};


⸻

9. Server Stack

namespace uds::server
{
    class Server
    {
    public:
        explicit Server(transport::ITransport& transport);

        void register_service(core::IService& service);

        void poll();

    private:
        transport::ITransport& transport_;
        Dispatcher dispatcher_;
    };
}

Server::poll()

void Server::poll()
{
    std::vector<uint8_t> rx;
    if (!transport_.receive(rx, 100))
        return;

    auto req = core::parse_request(rx);
    auto resp = dispatcher_.dispatch(req);
    transport_.send(core::serialize_response(resp));
}


⸻

10. Client API

namespace uds::client
{
    class Client
    {
    public:
        explicit Client(transport::ITransport& transport);

        core::Response request(const core::Request& req,
                               uint32_t timeout_ms);
    };
}


⸻

11. Error Handling Strategy

Mechanism	Use
NRC	Protocol errors
bool return	Transport failures
std::optional	Decoding failures
No exceptions	Embedded compatibility


⸻

12. Configuration & Timing

struct TimingConfig
{
    uint32_t p2_ms;
    uint32_t p2_star_ms;
    uint32_t s3_ms;
};

Used by:
	•	Session handling
	•	Response pending
	•	Client timeouts

⸻

13. Testing Strategy

✔ Mock ITransport
✔ Unit test each service
✔ Fuzz request decoding
✔ SIL/HIL compatible

Example mock:

class MockTransport : public uds::transport::ITransport
{
public:
    bool send(const std::vector<uint8_t>&) override;
    bool receive(std::vector<uint8_t>&, uint32_t) override;
};


⸻

14. Optional Enhancements

🔹 Security Access framework (seed/key plugin)
🔹 Asynchronous server loop
🔹 Memory-constrained static buffers
🔹 DoIP transport
🔹 Service auto-registration
🔹 Compile-time service tables

⸻

15. Summary

This design provides:

✅ ISO-14229 correctness
✅ Transport independence
✅ Clear client/server separation
✅ Embedded + PC compatibility
✅ Easy extensibility

⸻
I'll analyze the current UDS service implementation and provide recommendations for additional services that would be valuable for a comprehensive DoIP server library, especially for simulation and testing purposes.

[](file:///Users/oliverwieland/dev/doip-server/inc/uds/UdsServices.h#1-1) Zeilen 1 bis 100 lesen

[](file:///Users/oliverwieland/dev/doip-server/src/uds/UdsMock.cpp#1-1) Zeilen 1 bis 65 lesen

[](file:///Users/oliverwieland/dev/doip-server/inc/uds/services) lesen

Nach Text `UdsService::` (`**/inc/uds/UdsServices.h`) gesucht, 20 Ergebnisse

Based on my analysis of the current implementation, here's my recommendation for additional UDS services:

## Current Implementation Status

**Already Implemented (8 services):**
- ✅ 0x10 DiagnosticSessionControl
- ✅ 0x11 ECUReset
- ✅ 0x22 ReadDataByIdentifier
- ✅ 0x2E WriteDataByIdentifier
- ✅ 0x34 RequestDownload
- ✅ 0x36 TransferData
- ✅ 0x37 RequestTransferExit
- ✅ 0x3E TesterPresent

**Headers Exist, But Not Registered (14 services):**
All these have skeleton headers in services but return "ServiceNotSupported" by default.

## Recommended Additional Services

### **High Priority** (Essential for most diagnostic scenarios) 🔥

#### 1. **0x27 SecurityAccess**
**Why:** Required for protected diagnostic functions (reprogramming, calibration)
```cpp
// Use cases:
- ECU reprogramming authorization
- Protected DTC clearing
- Calibration data access
- Security-critical operations
```
**Complexity:** Medium (requires seed/key challenge-response)
**Student Value:** ⭐⭐⭐⭐⭐ Essential for understanding automotive security

---

#### 2. **0x19 ReadDTCInformation**
**Why:** Core diagnostic feature - reading fault codes
```cpp
// Sub-functions needed:
- 0x01: reportNumberOfDTCByStatusMask
- 0x02: reportDTCByStatusMask
- 0x04: reportDTCSnapshotRecordByDTCNumber
- 0x06: reportDTCExtendedDataRecordByDTCNumber
- 0x0A: reportSupportedDTC
```
**Complexity:** High (multiple sub-functions, DTC storage needed)
**Student Value:** ⭐⭐⭐⭐⭐ Most common diagnostic operation

---

#### 3. **0x14 ClearDiagnosticInformation**
**Why:** Pairs with 0x19, required after repairs
```cpp
// Use case:
- Clear stored DTCs
- Clear freeze frame data
- Reset emission-related data
```
**Complexity:** Low (simple DTC mask parameter)
**Student Value:** ⭐⭐⭐⭐ Completes DTC management workflow

---

### **Medium Priority** (Common in production) 🔨

#### 4. **0x28 CommunicationControl**
**Why:** Control ECU communication for reprogramming/diagnostics
```cpp
// Use cases:
- Disable normal CAN messages during reprogramming
- Enable/disable Rx/Tx selectively
- Network management control
```
**Complexity:** Medium (affects message transmission behavior)
**Student Value:** ⭐⭐⭐⭐ Important for understanding ECU behavior

---

#### 5. **0x85 ControlDTCSetting**
**Why:** Temporarily disable DTC logging during testing
```cpp
// Use cases:
- Disable DTC logging during EOL testing
- Prevent false DTCs during calibration
- Test-mode operation
```
**Complexity:** Low
**Student Value:** ⭐⭐⭐ Useful for test scenarios

---

#### 6. **0x23 ReadMemoryByAddress**
**Why:** Direct memory access for debugging/calibration
```cpp
// Use cases:
- Read calibration parameters
- Access debug data structures
- Memory verification after flashing
```
**Complexity:** Medium (requires memory protection checks)
**Student Value:** ⭐⭐⭐⭐ Good for learning memory access patterns

---

#### 7. **0x3D WriteMemoryByAddress**
**Why:** Pairs with 0x23, used for calibration
```cpp
// Use cases:
- Write calibration parameters
- Patch code in RAM
- Update configuration data
```
**Complexity:** Medium-High (security critical, needs protection)
**Student Value:** ⭐⭐⭐ Advanced but important

---

### **Low Priority** (Specialized use cases) 💡

#### 8. **0x2A ReadDataByPeriodicIdentifier**
**Why:** Periodic data transmission (e.g., live data streaming)
```cpp
// Use cases:
- Real-time sensor monitoring
- Live data graphing in diagnostic tools
- Performance tuning
```
**Complexity:** High (requires background task management)
**Student Value:** ⭐⭐⭐ Cool feature but complex

---

#### 9. **0x2C DynamicallyDefineDataIdentifier**
**Why:** Custom composite DID creation
```cpp
// Use cases:
- Group multiple DIDs into one read
- Optimize bandwidth for monitoring
- Create custom data views
```
**Complexity:** High (runtime DID management)
**Student Value:** ⭐⭐ Advanced feature

---

#### 10. **0x31 RoutineControl** *(Not in your enum yet!)*
**Why:** Execute ECU-specific procedures
```cpp
// Use cases:
- Actuator tests (window motor, valve solenoid)
- Self-tests and calibration routines
- Erase memory routines
```
**Complexity:** High (ECU-specific implementations)
**Student Value:** ⭐⭐⭐⭐ Very practical for HIL testing

---

## My Recommendation Priority

For a **comprehensive simulation library**, implement in this order:

### Phase 1: Core Diagnostics (2-3 weeks)
1. **0x27 SecurityAccess** - Most requested feature
2. **0x19 ReadDTCInformation** - Essential for any diagnostic tool
3. **0x14 ClearDiagnosticInformation** - Completes DTC workflow

### Phase 2: Advanced Operations (1-2 weeks)
4. **0x28 CommunicationControl** - Needed for reprogramming scenarios
5. **0x85 ControlDTCSetting** - Test mode control
6. **0x31 RoutineControl** - ECU-specific operations *(add to enum first)*

### Phase 3: Memory Access (1-2 weeks)
7. **0x23 ReadMemoryByAddress** - Direct memory access
8. **0x3D WriteMemoryByAddress** - Calibration support

### Phase 4: Specialized (optional)
9. **0x2A ReadDataByPeriodicIdentifier** - Live data streaming
10. **0x2C DynamicallyDefineDataIdentifier** - Advanced optimization

## Why These Matter for Students

**Scenario 1: ECU Simulation Project**
```
Student needs: 0x10, 0x27, 0x22, 0x19, 0x14, 0x3E
→ Can simulate realistic ECU with security + DTC management
```

**Scenario 2: Automated Test Framework**
```
Student needs: 0x10, 0x27, 0x31, 0x22, 0x2E, 0x28
→ Can run actuator tests and change calibration
```

**Scenario 3: Reprogramming Simulation**
```
Student needs: 0x10, 0x27, 0x28, 0x34, 0x36, 0x37, 0x31
→ Complete flash reprogramming workflow
```

## Services You Can Skip (for now)

- **0x83 AccessTimingParameters** - Rarely used
- **0x84 SecuredDataTransmission** - Complex security protocol
- **0x86 ResponseOnEvent** - Event-driven programming
- **0x87 LinkControl** - Baudrate switching (obsolete for DoIP)
- **0x24 ReadScalingDataByIdentifier** - Legacy ASAM-2 MC support

## Summary

**Must-have additions:**
- 0x27 SecurityAccess ⭐⭐⭐⭐⭐
- 0x19 ReadDTCInformation ⭐⭐⭐⭐⭐
- 0x14 ClearDiagnosticInformation ⭐⭐⭐⭐

**Should-have additions:**
- 0x28 CommunicationControl
- 0x85 ControlDTCSetting
- 0x31 RoutineControl (need to add to enum)

This gives you **11 total services** covering 90% of real-world diagnostic scenarios for simulation and testing purposes.