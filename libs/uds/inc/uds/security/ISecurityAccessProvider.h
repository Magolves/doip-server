#ifndef ISECURITYPROVIDER_H
#define ISECURITYPROVIDER_H

#include <chrono>
#include <map>

#include "uds/UdsResponseCode.h"
#include "util/ByteArray.h"

namespace doip::uds::security {

class ISecurityAccessProvider {
  public:
    virtual ~ISecurityAccessProvider() = default;

    virtual std::string_view  getName() const noexcept = 0;

    /**
     * @brief Generate a seed for the given security securityLevel
     *
     * @param securityLevel Security securityLevel
     * @param seed Output parameter to hold the generated seed
     * @return UdsResponseCode indicating success or failure
     */
    virtual UdsResponseCode generateSeed(uint8_t securityLevel, ByteArray &seed) {
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
        // if (!isSecurityAccessDelayExpired(securityLevel)) {
        //     return UdsResponseCode::RequiredTimeDelayNotExpired;
        // }

        auto rc = generateSeedImpl(securityLevel, seed);
        std::cout << "Generate seed  " << seed << " for level " << +securityLevel << "'\n";
        return rc;
    }

    virtual UdsResponseCode verifyKey(uint8_t securityLevel, const ByteArray &key) {
        std::cout << "Seed provider 1: Verify key '" << key << "' for level " << +securityLevel << "\n";

        // Check if seed was requested first
        if (!hasPendingSeed(securityLevel)) {
            return UdsResponseCode::RequestSequenceError;
        }

        // Check attempt counter
        if (getSecurityAccessAttempts(securityLevel) >= getMaxSecurityAttempts()) {
            return UdsResponseCode::ExceededNumberOfAttempts;
        }

        // Verify key (must be overridden by derived class)
        bool valid = this->verifyKeyImpl(securityLevel, key);

        if (valid) {
            onKeyVerified(securityLevel);
            return UdsResponseCode::PositiveResponse;
        } else {
            incrementSecurityAccessAttempts(securityLevel);
            onKeyFailed(securityLevel);

            return UdsResponseCode::InvalidKey;
        }
    }


    // Security Access State Query Methods

    /**
     * @brief Check if a specific security securityLevel is currently unlocked
     */
    virtual bool isSecurityLevelUnlocked(uint8_t securityLevel) const {
        auto it = m_unlockedSecurityLevels.find(securityLevel);
        return it != m_unlockedSecurityLevels.end() && it->second;
    }

    /**
     * @brief Lock all security levels (called on session change or ECU reset)
     */
    virtual void lockAllSecurityLevels() {
        m_unlockedSecurityLevels.clear();
        m_securityAttempts.clear();
        m_securityDelayExpiry.clear();
        m_pendingSeeds.clear();
    }

    /**
     * @brief Get number of failed attempts for a security securityLevel
     */
    virtual uint8_t getSecurityAccessAttempts(uint8_t securityLevel) const {
        auto it = m_securityAttempts.find(securityLevel);
        return it != m_securityAttempts.end() ? it->second : 0;
    }

    /**
     * @brief Get maximum allowed attempts (typically 3-5)
     */
    virtual uint8_t getMaxSecurityAttempts() const {
        return 3;
    }

    /**
     * @brief Get delay duration after exceeding attempts (typically 10 seconds)
     */
    virtual std::chrono::seconds getSecurityAccessDelay() const {
        return std::chrono::seconds(10);
    }

    size_t getStoredSeedCount() const {
        return m_pendingSeeds.size();
    }

    /**
     * @brief Get expected seed length for a security level (typically 4 bytes)
     */
    virtual size_t getSeedLength(uint8_t level) const {
        (void)level;
        return 4; // Default: 4-byte seed
    }


  protected:
    virtual UdsResponseCode generateSeedImpl(uint8_t securityLevel, ByteArray &seed) = 0;

    /**
     * @brief Verify the provided key against the expected key for the given security securityLevel
     *
     * @param securityLevel Security securityLevel
     * @param key The key to verify
     * @return true if the key is valid, false otherwise
     */
    virtual bool verifyKeyImpl(uint8_t securityLevel, const ByteArray &key) = 0;

    /**
     * @brief Callback invoked when a key is successfully verified
     *
     * @param securityLevel the security securityLevel that was unlocked
     */
    virtual void onKeyVerified(uint8_t securityLevel) {
        unlockSecurityLevel(securityLevel);
        resetSecurityAccessAttempts(securityLevel);
    }

    /**
     * @brief Callback invoked when a key verification fails
     *
     * @param securityLevel the security securityLevel that failed to unlock
     */
    virtual void onKeyFailed(uint8_t securityLevel) {
        incrementSecurityAccessAttempts(securityLevel);
        startSecurityAccessDelay(securityLevel);
    }

    // Internal state management methods

    void unlockSecurityLevel(uint8_t securityLevel) {
        m_unlockedSecurityLevels[securityLevel] = true;
        m_unlockedSecurityLevels[securityLevel + 1] = true; // Also unlock the verifyKey securityLevel
    }

    void incrementSecurityAccessAttempts(uint8_t securityLevel) {
        std::cout << "Attempts L" << +securityLevel << ": " << m_securityAttempts[securityLevel] << "\n";
        m_securityAttempts[securityLevel]++;
    }

    void resetSecurityAccessAttempts(uint8_t securityLevel) {
        std::cout << "Reset Attempts L" << +securityLevel << ": " << m_securityAttempts[securityLevel] << "\n";
        m_securityAttempts[securityLevel] = 0;
    }

    void startSecurityAccessDelay(uint8_t securityLevel) {
        m_securityDelayExpiry[securityLevel] = std::chrono::steady_clock::now() + getSecurityAccessDelay();
    }

    bool isSecurityAccessDelayExpired(uint8_t securityLevel) const {
        auto it = m_securityDelayExpiry.find(securityLevel);
        if (it == m_securityDelayExpiry.end()) {
            return true; // No delay active
        }
        return std::chrono::steady_clock::now() >= it->second;
    }

    // Seed mgmt methods

    bool hasPendingSeed(uint8_t securityLevel) const {
        return m_pendingSeeds.find(securityLevel) != m_pendingSeeds.end();
    }

    void storePendingSeed(uint8_t securityLevel, const ByteArray &seed) {
        m_pendingSeeds[securityLevel] = seed;
    }

    void clearPendingSeed(uint8_t securityLevel) {
        m_pendingSeeds.erase(securityLevel);
    }

    const std::optional<ByteArray> getPendingSeed(uint8_t securityLevel) const {
        auto it = m_pendingSeeds.find(securityLevel);
        return it != m_pendingSeeds.end() ? std::optional<ByteArray>(it->second) : std::nullopt;
    }

  private:
    // Security Access State
    std::map<uint8_t, bool> m_unlockedSecurityLevels;                               // securityLevel -> unlocked
    std::map<uint8_t, uint8_t> m_securityAttempts;                                  // securityLevel -> attempt count
    std::map<uint8_t, std::chrono::steady_clock::time_point> m_securityDelayExpiry; // securityLevel -> delay expiry time

    std::map<uint8_t, ByteArray> m_pendingSeeds; // securityLevel -> last sent seed
};

} // namespace doip::uds::security

#endif /* ISECURITYPROVIDER_H */
