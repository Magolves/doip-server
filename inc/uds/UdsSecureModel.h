#ifndef UDSSECUREMODEL_H
#define UDSSECUREMODEL_H

#include "UdsDefaultModel.h"
#include <random>

namespace doip::uds {

/**
 * @brief Example UDS model with security access implementation
 *
 * This demonstrates a simple seed-key algorithm suitable for simulation/testing.
 *
 * Security Levels:
 * - Level 1 (0x01/0x02): Programming Session Access
 * - Level 2 (0x03/0x04): Extended Diagnostic Access
 *
 * WARNING: This uses a simplified algorithm for educational purposes.
 * Production systems should use cryptographic algorithms (AES, etc.) and HSM.
 */
class UdsSecureModel : public UdsDefaultModel {
public:
    UdsSecureModel() : m_randomEngine(std::random_device{}()) {}

    ~UdsSecureModel() override = default;

    std::string_view getModelName() const override {
        return "UDS Secure Model (Example)";
    }

protected:
    /**
     * @brief Generate a random 4-byte seed
     */
    UdsResponseCode generateSeed(uint8_t level, ByteArray &seed) override {
        (void)level; // Same seed generation for all levels

        // Generate 4 random bytes
        seed.clear();
        std::uniform_int_distribution<uint32_t> dist(0x00000001, 0xFFFFFFFE);
        uint32_t randomSeed = dist(m_randomEngine);

        // Store for later verification
        m_lastSeed = randomSeed;

        // Convert to bytes (big-endian)
        seed.writeU32(randomSeed);

        // Mark as pending
        storePendingSeed(level, seed);

        return UdsResponseCode::PositiveResponse;
    }

    /**
     * @brief Verify key using simple transformation algorithm
     *
     * This demonstrates three common approaches:
     * - Level 1: XOR + constant addition
     * - Level 3: Bit rotation + mask
     *
     * Real implementations should use:
     * - AES encryption
     * - RSA signing
     * - OEM-specific proprietary algorithms
     * - Hardware Security Module (HSM)
     */
    bool verifyKey(uint8_t level, const ByteArray &key) override {
        // Key must be 4 bytes for our algorithm
        if (key.size() != 4) {
            return false;
        }

        const ByteArray* pendingSeed = getPendingSeed(level);
        if (!pendingSeed || pendingSeed->size() != 4) {
            return false;
        }

        uint32_t seedValue = pendingSeed->readU32(0);
        uint32_t keyValue = key.readU32(0);
        uint32_t expectedKey = 0;

        switch (level) {
            case 1: // Programming session
                // Simple XOR with constant and addition
                expectedKey = (seedValue ^ SECURITY_CONSTANT_1) + SECURITY_OFFSET_1;
                break;

            case 3: // Extended diagnostic
                // Bit rotation and mask
                expectedKey = rotateLeft(seedValue, 5) ^ SECURITY_MASK_2;
                break;

            case 5: // Custom security level
                // More complex transformation
                expectedKey = ((seedValue * SECURITY_MULTIPLIER) ^ seedValue) + SECURITY_OFFSET_2;
                break;

            default:
                return false; // Unsupported level
        }

        return keyValue == expectedKey;
    }

private:
    // Security constants (in production, these would be secret)
    static constexpr uint32_t SECURITY_CONSTANT_1 = 0xA5A5A5A5;
    static constexpr uint32_t SECURITY_OFFSET_1 = 0x12345678;
    static constexpr uint32_t SECURITY_MASK_2 = 0x5A5A5A5A;
    static constexpr uint32_t SECURITY_MULTIPLIER = 0x9D2C5680;
    static constexpr uint32_t SECURITY_OFFSET_2 = 0x87654321;

    std::mt19937 m_randomEngine;
    uint32_t m_lastSeed = 0;

    // Helper function for bit rotation
    static uint32_t rotateLeft(uint32_t value, uint8_t bits) {
        return (value << bits) | (value >> (32 - bits));
    }
};

} // namespace doip::uds

#endif /* UDSSECUREMODEL_H */
