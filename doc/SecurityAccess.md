# UDS Security Access Implementation Guide

## Overview

The security access service (SID 0x27) is a critical UDS service that controls access to protected diagnostic functions like ECU reprogramming, calibration data modification, and DTC clearing.

This implementation follows **ISO 14229-1:2020** specifications and provides a flexible framework for implementing seed-key algorithms.

## Architecture

### Components

```
IUdsModel (Abstract Base)
    ├── Security state management (attempts, delays, unlocked levels)
    ├── Abstract methods: generateSeed(), verifyKey()
    └── Template method pattern for security flow

SecurityAccessHandler (Service Handler)
    ├── Request parsing and validation
    ├── Sub-function routing (requestSeed/sendKey)
    └── Response generation

UdsSecureModel (Concrete Implementation)
    ├── Random seed generation
    └── Example seed-key algorithms (XOR, rotation, etc.)
```

## Security Access Flow

### Standard Workflow

```
Client                                  ECU
  │                                      │
  ├─────(1) Request Seed (0x27 0x01)───→│
  │                                      │ Generate random seed
  │                                      │ Store seed internally
  │                                      │
  │←────(2) Seed Response (0x67 0x01)───┤
  │         + 4 bytes seed                │
  │                                      │
  │ Calculate Key = f(seed)              │
  │                                      │
  ├─────(3) Send Key (0x27 0x02)────────→│
  │         + 4 bytes key                 │
  │                                      │ Verify key matches expected
  │                                      │
  │←────(4) Positive Response (0x67 0x02)┤
  │                                      │
  │ Level unlocked - can access         │
  │ protected services                   │
```

### Security Levels

| Level | Sub-function | Purpose | Typical Use Case |
|-------|--------------|---------|------------------|
| 1 | 0x01 (seed) / 0x02 (key) | Programming Session | ECU reprogramming, firmware updates |
| 2 | 0x03 (seed) / 0x04 (key) | Extended Diagnostic | Advanced diagnostics, calibration |
| 3 | 0x05 (seed) / 0x06 (key) | Safety/Security | Safety-critical operations |
| ... | ... | ... | Manufacturer-specific levels |

**Note:** Odd sub-functions are for requesting seed, even sub-functions are for sending key.

## Implementation Guide

### Step 1: Implement Your UDS Model

```cpp
#include "uds/UdsSecureModel.h"

class MyEcuModel : public UdsDefaultModel {
protected:
    // Generate a unique seed (4 bytes recommended)
    UdsResponseCode generateSeed(uint8_t level, ByteArray &seed) override {
        // Option 1: Random seed (recommended)
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint32_t> dist(1, 0xFFFFFFFE);
        uint32_t randomSeed = dist(gen);
        seed.writeU32(randomSeed);

        // Store for verification
        storePendingSeed(level, seed);

        return UdsResponseCode::PositiveResponse;
    }

    // Verify the key sent by the tester
    bool verifyKey(uint8_t level, const ByteArray &key) override {
        const ByteArray* seed = getPendingSeed(level);
        if (!seed || seed->size() != 4 || key.size() != 4) {
            return false;
        }

        uint32_t seedValue = seed->readU32(0);
        uint32_t keyValue = key.readU32(0);
        uint32_t expectedKey = calculateKey(level, seedValue);

        return keyValue == expectedKey;
    }

private:
    uint32_t calculateKey(uint8_t level, uint32_t seed) {
        // YOUR ALGORITHM HERE
        // Example: Simple XOR for demonstration only
        const uint32_t SECRET = 0xDEADBEEF;
        return seed ^ SECRET;
    }
};
```

### Step 2: Common Seed-Key Algorithms

#### Simple XOR + Addition (Educational)
```cpp
uint32_t calculateKey(uint32_t seed) {
    const uint32_t XOR_MASK = 0xA5A5A5A5;
    const uint32_t OFFSET = 0x12345678;
    return (seed ^ XOR_MASK) + OFFSET;
}
```

#### Bit Rotation + XOR (Moderate)
```cpp
uint32_t calculateKey(uint32_t seed) {
    // Rotate left 5 bits
    uint32_t rotated = (seed << 5) | (seed >> 27);
    const uint32_t MASK = 0x5A5A5A5A;
    return rotated ^ MASK;
}
```

#### Multi-stage Transformation (Advanced)
```cpp
uint32_t calculateKey(uint32_t seed) {
    uint32_t stage1 = seed ^ 0x12345678;
    uint32_t stage2 = (stage1 * 0x9D2C5680) & 0xFFFFFFFF;
    uint32_t stage3 = stage2 + ((seed >> 16) ^ (seed << 16));
    return stage3 ^ 0x87654321;
}
```

⚠️ **Production Warning:** These are educational examples only. Production systems should use:
- AES-128/256 encryption
- RSA signing
- Hardware Security Module (HSM)
- OEM-proprietary algorithms with secure key storage

### Step 3: Configure Security Parameters

```cpp
class MyEcuModel : public UdsDefaultModel {
public:
    // Override security parameters
    uint8_t getMaxSecurityAttempts() const override {
        return 5; // Allow 5 attempts (default: 3)
    }

    std::chrono::seconds getSecurityAccessDelay() const override {
        return std::chrono::seconds(30); // 30 second delay (default: 10)
    }

    size_t getSeedLength(uint8_t level) const override {
        // Different levels can have different seed lengths
        return level <= 2 ? 4 : 8; // 4 bytes for level 1-2, 8 bytes for level 3+
    }
};
```

## ISO 14229 Compliance

### Required Behaviors

✅ **Already unlocked:** If security level is unlocked, `requestSeed` returns all zeros
✅ **Session change:** All security levels are locked when diagnostic session changes
✅ **ECU reset:** All security levels are locked on reset
✅ **Attempt counter:** Track failed attempts, lock after exceeding limit
✅ **Delay timer:** Enforce delay after exceeding attempts
✅ **Sequence check:** `sendKey` must follow `requestSeed`

### Negative Response Codes

| NRC | Code | Condition |
|-----|------|-----------|
| InvalidKey | 0x35 | Key verification failed |
| ExceededNumberOfAttempts | 0x36 | Too many failed attempts |
| RequiredTimeDelayNotExpired | 0x37 | Attempt made during delay period |
| RequestSequenceError | 0x24 | sendKey without requestSeed |
| SubFunctionNotSupported | 0x12 | Invalid security level |
| SecurityAccessDenied | 0x33 | Access denied (e.g., wrong session) |

## Testing Examples

### Test Case 1: Successful Unlock
```cpp
// Request seed
ByteArray seedReq = {0x27, 0x01};
ByteArray seedRsp = handler.handle(seedReq, model);

// Extract seed
ByteArray seed(seedRsp.begin() + 2, seedRsp.end());
uint32_t seedValue = seed.readU32(0);

// Calculate key
uint32_t keyValue = calculateKey(seedValue);

// Send key
ByteArray keyReq = {0x27, 0x02};
keyReq.writeU32(keyValue);
ByteArray keyRsp = handler.handle(keyReq, model);

// Verify success
assert(keyRsp[0] == 0x67); // Positive response
assert(model->isSecurityLevelUnlocked(1));
```

### Test Case 2: Failed Attempts
```cpp
for (int i = 0; i < 3; i++) {
    handler.handle({0x27, 0x01}, model); // Request seed
    handler.handle({0x27, 0x02, 0x00, 0x00, 0x00, 0x00}, model); // Wrong key
}

// Fourth attempt should fail
ByteArray response = handler.handle({0x27, 0x01}, model);
assert(response[2] == 0x36); // ExceededNumberOfAttempts
```

## Integration with DoIP Server

```cpp
// In your DoIPServerModel
class MyDoIPServerModel : public DoIPDownstreamServerModel {
public:
    MyDoIPServerModel()
        : DoIPDownstreamServerModel("my-ecu", m_udsProvider),
          m_udsProvider(std::make_unique<MyEcuModel>())
    {
        // Security is automatically available via UdsMock
    }

private:
    UdsMockProvider m_udsProvider;
};
```

## Security Best Practices

### DO ✅
- Use cryptographically secure random number generators
- Implement proper key derivation functions
- Use HSM for key storage and verification when available
- Log security access attempts for audit trail
- Implement time-based seed expiration
- Use different algorithms for different security levels
- Validate seed/key lengths strictly

### DON'T ❌
- Hardcode seeds or use predictable patterns
- Use simple XOR/addition in production
- Allow unlimited retry attempts
- Store keys in plain text
- Reuse the same seed
- Accept keys without prior seed request
- Skip delay enforcement after exceeding attempts

## Troubleshooting

### "RequestSequenceError" on sendKey
**Problem:** Tried to send key without requesting seed first
**Solution:** Always request seed (0x27 0x01) before sending key (0x27 0x02)

### "ExceededNumberOfAttempts" on requestSeed
**Problem:** Too many failed key attempts
**Solution:** Wait for delay period (default 10 seconds) or reset ECU

### Key calculation returns wrong value
**Problem:** Algorithm mismatch between tester and ECU
**Solution:** Verify both sides use identical transformation logic

### Security unlocked but still getting "SecurityAccessDenied"
**Problem:** Protected service requires higher security level
**Solution:** Check which security level the service requires

## Advanced Topics

### Multiple Security Levels
```cpp
// Check required level for operations
bool canFlash = model->isSecurityLevelUnlocked(1);
bool canCalibrate = model->isSecurityLevelUnlocked(3);
```

### Event Monitoring
```cpp
model->registerEventHandler([](UdsModelEvent event, const IUdsModel& m, const IModelEventData&) {
    switch (event) {
        case UdsModelEvent::SecurityAccessGranted:
            std::cout << "Security unlocked!\n";
            break;
        case UdsModelEvent::SecurityAccessDenied:
            std::cout << "Invalid key attempt\n";
            break;
        case UdsModelEvent::SecurityAccessAttemptExceeded:
            std::cout << "Too many attempts - delay active\n";
            break;
    }
});
```

### Session-Specific Security
```cpp
UdsResponseCode generateSeed(uint8_t level, ByteArray &seed) override {
    // Only allow certain levels in specific sessions
    DiagnosticSessionControlType session = getCurrentSession();

    if (level == 1 && session != DiagnosticSessionControlType::ProgrammingSession) {
        return UdsResponseCode::SecurityAccessDenied;
    }

    // Generate seed...
}
```

## References

- ISO 14229-1:2020 - Unified Diagnostic Services (UDS)
- ISO 13400-2:2019 - DoIP Protocol Specification
- AUTOSAR SWS DiagnosticOverIP
- SAE J2534 - PassThru Interface
