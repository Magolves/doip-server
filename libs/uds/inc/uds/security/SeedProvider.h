#ifndef SEEDPROVIDER_H
#define SEEDPROVIDER_H

#include "uds/gen/UdsConfig.h"
#include "uds/security/ISecurityAccessProvider.h"
#include "util/ByteArray.h"

#include <random>

namespace doip::uds::security {

class SeedProvider : public ISecurityAccessProvider {
    public:
    virtual ~SeedProvider() = default;

    /**
     * @brief Generate a random 4-byte seed
     */
    UdsResponseCode generateSeed(uint8_t securityLevel, ByteArray &seed) override {
        // Check if already unlocked
        if (isSecurityLevelUnlocked(securityLevel)) {
            // ISO 14229: Return all zeros if already unlocked
            seed.clear();
            seed.resize(getSeedLength(securityLevel), 0x00);
            return UdsResponseCode::PositiveResponse;
        }

        // Check attempt counter
        if (getSecurityAccessAttempts(securityLevel) >= getMaxSecurityAttempts()) {
            return UdsResponseCode::ExceededNumberOfAttempts;
        }

        // Check delay timer
        if (!isSecurityAccessDelayExpired(securityLevel)) {
            return UdsResponseCode::RequiredTimeDelayNotExpired;
        }

        // Generate 4 random bytes
        seed.clear();
        std::uniform_int_distribution<uint32_t> dist(0x00000001, 0xFFFFFFFE);
        uint32_t randomSeed = dist(m_randomEngine);

        // Store for later verification
        m_lastSeed = randomSeed;

        // Convert to bytes (big-endian)
        seed.writeU32(randomSeed);

        // Mark as pending
        storePendingSeed(securityLevel, seed);

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

        const ByteArray *pendingSeed = getPendingSeed(level);
        if (!pendingSeed || pendingSeed->size() != 4) {
            return false;
        }

        uint32_t seedValue = pendingSeed->readU32(0);
        uint32_t keyValue = key.readU32(0);
        uint32_t expectedKey = 0;

        switch (level) {
        case 1: // Programming session
            // Simple XOR with constant and addition
            expectedKey = (seedValue ^ UDS_SEED_CONSTANT_1) + UDS_SEED_OFFSET_1;
            break;

        case 3: // Extended diagnostic
            // Bit rotation and mask
            expectedKey = rotateLeft(seedValue, 5) ^ UDS_SEED_MASK_2;
            break;

        case 5: // Custom security level
            // More complex transformation
            expectedKey = ((seedValue * UDS_SEED_MULTIPLIER) ^ seedValue) + UDS_SEED_OFFSET_2;
            break;

        default:
            return false; // Unsupported level
        }

        return keyValue == expectedKey;
    }

    virtual void onKeyVerified(uint8_t securityLevel) override {
        ISecurityAccessProvider::onKeyVerified(securityLevel);
        clearPendingSeed(securityLevel);
    }

    virtual void onKeyFailed(uint8_t securityLevel) override {
        ISecurityAccessProvider::onKeyFailed(securityLevel);
        clearPendingSeed(securityLevel);
    }

    /**
     * @brief Get expected seed length for a security level (typically 4 bytes)
     */
    virtual size_t getSeedLength(uint8_t level) const {
        (void)level;
        return 4; // Default: 4-byte seed
    }

  protected:


  private:
    // Security constants (in production, these would be secret)
    static constexpr uint32_t UDS_SEED_CONSTANT_1 = UDS_CONFIG_SEED_CONSTANT_1;
    static constexpr uint32_t UDS_SEED_OFFSET_1 = UDS_CONFIG_SEED_OFFSET_1;
    static constexpr uint32_t UDS_SEED_MASK_2 = UDS_CONFIG_SEED_MASK_2;
    static constexpr uint32_t UDS_SEED_MULTIPLIER = UDS_CONFIG_SEED_MULTIPLIER;
    static constexpr uint32_t UDS_SEED_OFFSET_2 = UDS_CONFIG_SEED_OFFSET_2;

    std::mt19937 m_randomEngine;
    uint32_t m_lastSeed = 0;

    // Helper function for bit rotation
    static uint32_t rotateLeft(uint32_t value, uint8_t bits) {
        return (value << bits) | (value >> (32 - bits));
    }
};

} // namespace doip::uds::security

#endif /* SEEDPROVIDER_H */
