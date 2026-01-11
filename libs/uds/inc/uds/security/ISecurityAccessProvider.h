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

    /**
     * @brief Generate a seed for the given security securityLevel
     *
     * @param securityLevel Security securityLevel
     * @param seed Output parameter to hold the generated seed
     * @return UdsResponseCode indicating success or failure
     */
    virtual UdsResponseCode generateSeed(uint8_t securityLevel, ByteArray &seed) = 0;

    virtual UdsResponseCode sendKey(uint8_t securityLevel, const ByteArray &key) {
        uint8_t seedLevel = securityLevel - 1; // sendKey uses even, seed uses odd

        // Check if seed was requested first
        if (!hasPendingSeed(seedLevel)) {
            return UdsResponseCode::RequestSequenceError;
        }

        // Check attempt counter
        if (getSecurityAccessAttempts(seedLevel) >= getMaxSecurityAttempts()) {
            return UdsResponseCode::ExceededNumberOfAttempts;
        }

        // Verify key (must be overridden by derived class)
        bool valid = verifyKey(seedLevel, key);

        if (valid) {
            onKeyVerified(seedLevel);
            // clearPendingSeed(seedLevel);

            // if (m_eventHandler) {
            //     m_eventHandler(UdsModelEvent::SecurityAccessGranted, *this, IModelEventData{});
            // }

            return UdsResponseCode::PositiveResponse;
        } else {
            incrementSecurityAccessAttempts(seedLevel);
            // clearPendingSeed(seedLevel);

            // if (m_eventHandler) {
            //     if (getSecurityAccessAttempts(seedLevel) >= getMaxSecurityAttempts()) {
            //         m_eventHandler(UdsModelEvent::SecurityAccessAttemptExceeded, *this, IModelEventData{});

            //     } else {
            //         m_eventHandler(UdsModelEvent::SecurityAccessDenied, *this, IModelEventData{});
            //     }
            // }
            onKeyFailed(securityLevel);

            return UdsResponseCode::InvalidKey;
        }
    }

    /**
     * @brief Verify the provided key against the expected key for the given security securityLevel
     *
     * @param securityLevel Security securityLevel
     * @param key The key to verify
     * @return true if the key is valid, false otherwise
     */
    virtual bool verifyKey(uint8_t securityLevel, const ByteArray &key) = 0;

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

  protected:
    // Internal state management methods

    void unlockSecurityLevel(uint8_t securityLevel) {
        m_unlockedSecurityLevels[securityLevel] = true;
        m_unlockedSecurityLevels[securityLevel + 1] = true; // Also unlock the sendKey securityLevel
    }

    void incrementSecurityAccessAttempts(uint8_t securityLevel) {
        m_securityAttempts[securityLevel]++;
    }

    void resetSecurityAccessAttempts(uint8_t securityLevel) {
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

    const ByteArray *getPendingSeed(uint8_t securityLevel) const {
        auto it = m_pendingSeeds.find(securityLevel);
        return it != m_pendingSeeds.end() ? &it->second : nullptr;
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
