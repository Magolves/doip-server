#ifndef IUDSMODEL_H
#define IUDSMODEL_H

#include "uds/UdsResponseCode.h"
#include "uds/UdsTypes.h"
#include "util/ByteArray.h"

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <string_view>

namespace doip::uds {

enum class UdsModelEvent : uint8_t {
    DiagnosticSessionChange,
    EcuReset,
    SecurityAccessGranted,
    SecurityAccessDenied,
    SecurityAccessAttemptExceeded,
    DataByIdentifierRead,
    DataByIdentifierWrite
};

/** Event data passed to UdsModelEventHandler callbacks */
struct IModelEventData {
};

class IUdsModel;

using UdsModelEventHandler = std::function<void(UdsModelEvent event, const IUdsModel &model, IModelEventData const &data)>;

class IUdsModel {
  public:
    IUdsModel(UdsModelEventHandler handler = nullptr, uint16_t p2_ms = 1000, uint16_t p2star_10ms = 500)
        : m_eventHandler(std::move(handler)), m_p2_ms(p2_ms), m_p2star_10ms(p2star_10ms) {}

    virtual ~IUdsModel() = default;

    void registerEventHandler(UdsModelEventHandler handler) {
        m_eventHandler = std::move(handler);
    }

    /**
     * @brief Get the P2 Timeout in milliseconds.
     *
     * @return uint16_t the P2 timeout in milliseconds.
     */
    uint16_t getP2TimeoutMs() const {
        return m_p2_ms;
    }

    /**
     * @brief Get the P2* Timeout in milliseconds.
     *
     * @return uint16_t the P2* timeout in milliseconds.
     */
    uint16_t getP2StarTimeoutMs() const {
        return m_p2star_10ms * 10;
    }

    /**
     * @brief Get the model name.
     * @return model name string.
     */
    virtual std::string_view getModelName() const = 0;

    /**
     * @brief Get the current diagnostic session.
     * @return current DiagnosticSessionControlType.
     */
    virtual DiagnosticSessionControlType getCurrentSession() const {
        return m_currentSession;
    }

    /**
     * @brief Set the current diagnostic session.
     * @param session new DiagnosticSessionControlType.
     *
     * Note: ISO 14229 requires locking all security levels on session change
     */
    virtual UdsResponseCode setCurrentSession(DiagnosticSessionControlType session) {
        if (m_currentSession == session) {
            return UdsResponseCode::ConditionsNotCorrect;
        }

        m_currentSession = session;

        // ISO 14229: Lock all security levels when changing sessions
        lockAllSecurityLevels();

        if (m_eventHandler) {
            m_eventHandler(UdsModelEvent::DiagnosticSessionChange, *this, IModelEventData{});
        }
        return UdsResponseCode::PositiveResponse;
    }

    /**
     * @brief Perform an ECU reset.
     *
     * @param resetType Type of ECU reset to perform.
     * @return UdsResponseCode indicating success or specific error condition.
     */
    virtual UdsResponseCode reset(EcuResetType resetType) {
        (void)resetType;
        return UdsResponseCode::PositiveResponse;
    }

    virtual bool supportsDataByIdentifier(uds_did did) const = 0;
    virtual UdsResponseCode getDataByIdentfier(uds_did did, ByteArray &data, size_t offset = 0) const = 0;
    virtual UdsResponseCode setDataByIdentfier(uds_did did, const ByteArray &data, size_t offset = 0) = 0;

    /**
     * @brief Request seed for security access (sub-function: requestSeed)
     *
     * @param securityLevel Security level requested (odd: requestSeed, even: sendKey)
     * @param seed [out] Generated seed/challenge to be sent to tester
     * @return UdsResponseCode indicating success or specific error condition
     *
     * Typical security levels:
     * - 0x01/0x02: Level 1 (Programming session access)
     * - 0x03/0x04: Level 2 (Extended diagnostic access)
     * - 0x05/0x06: Level 3 (Safety/security access)
     */
    virtual UdsResponseCode requestSeed(uint8_t securityLevel, ByteArray &seed) {
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

        // Generate seed (must be overridden by derived class)
        return generateSeed(securityLevel, seed);
    }

    /**
     * @brief Verify key for security access (sub-function: sendKey)
     *
     * @param securityLevel Security level to unlock (must be even: 0x02, 0x04, etc.)
     * @param key Key sent by tester to verify against expected response
     * @return UdsResponseCode indicating success or specific error condition
     */
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
            unlockSecurityLevel(seedLevel);
            resetSecurityAccessAttempts(seedLevel);
            clearPendingSeed(seedLevel);

            if (m_eventHandler) {
                m_eventHandler(UdsModelEvent::SecurityAccessGranted, *this, IModelEventData{});
            }

            return UdsResponseCode::PositiveResponse;
        } else {
            incrementSecurityAccessAttempts(seedLevel);
            clearPendingSeed(seedLevel);

            if (m_eventHandler) {
                if (getSecurityAccessAttempts(seedLevel) >= getMaxSecurityAttempts()) {
                    m_eventHandler(UdsModelEvent::SecurityAccessAttemptExceeded, *this, IModelEventData{});
                    startSecurityAccessDelay(seedLevel);
                } else {
                    m_eventHandler(UdsModelEvent::SecurityAccessDenied, *this, IModelEventData{});
                }
            }

            return UdsResponseCode::InvalidKey;
        }
    }

    virtual UdsResponseCode requestDownload(uint32_t memoryAddress, uint32_t memorySize, const ByteArray &transferParameters) {
        (void)memoryAddress;
        (void)memorySize;
        (void)transferParameters;
        return UdsResponseCode::ServiceNotSupported;
    }

    virtual UdsResponseCode transferData(uint8_t blockSequenceCounter, const ByteArray &data) {
        (void)blockSequenceCounter;
        (void)data;

        return UdsResponseCode::ServiceNotSupported;
    }

    virtual UdsResponseCode requestTransferExit() {
        return UdsResponseCode::ServiceNotSupported;
    }

    // Security Access State Query Methods

    /**
     * @brief Check if a specific security level is currently unlocked
     */
    virtual bool isSecurityLevelUnlocked(uint8_t level) const {
        auto it = m_unlockedSecurityLevels.find(level);
        return it != m_unlockedSecurityLevels.end() && it->second;
    }

    /**
     * @brief Lock all security levels (called on session change or ECU reset)
     */
    virtual void lockAllSecurityLevels() {
        m_unlockedSecurityLevels.clear();
        m_pendingSeeds.clear();
    }

    /**
     * @brief Get number of failed attempts for a security level
     */
    virtual uint8_t getSecurityAccessAttempts(uint8_t level) const {
        auto it = m_securityAttempts.find(level);
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

    /**
     * @brief Get expected seed length for a security level (typically 4 bytes)
     */
    virtual size_t getSeedLength(uint8_t level) const {
        (void)level;
        return 4; // Default: 4-byte seed
    }

  protected:
    // Methods to be implemented by derived classes

    /**
     * @brief Generate a seed/challenge for the tester
     *
     * Derived classes must implement their seed generation algorithm.
     * Common approaches:
     * - Random number generation
     * - Time-based seeds
     * - Counter-based seeds
     * - Pseudo-random with ECU-specific parameters
     */
    virtual UdsResponseCode generateSeed(uint8_t level, ByteArray &seed) {
        (void)level;
        (void)seed;
        return UdsResponseCode::ServiceNotSupported;
    }

    /**
     * @brief Verify the key sent by the tester
     *
     * Derived classes must implement their key verification algorithm.
     * Common approaches:
     * - Simple XOR/mathematical transformation
     * - AES/DES encryption
     * - Proprietary OEM algorithms
     * - HSM (Hardware Security Module) verification
     *
     * @param level Security level (odd number: 1, 3, 5, etc.)
     * @param key Key bytes sent by tester
     * @return true if key is valid, false otherwise
     */
    virtual bool verifyKey(uint8_t level, const ByteArray &key) {
        (void)level;
        (void)key;
        return false; // Deny by default
    }

    // Internal state management methods

    void unlockSecurityLevel(uint8_t level) {
        m_unlockedSecurityLevels[level] = true;
        m_unlockedSecurityLevels[level + 1] = true; // Also unlock the sendKey level
    }

    void incrementSecurityAccessAttempts(uint8_t level) {
        m_securityAttempts[level]++;
    }

    void resetSecurityAccessAttempts(uint8_t level) {
        m_securityAttempts[level] = 0;
    }

    bool hasPendingSeed(uint8_t level) const {
        return m_pendingSeeds.find(level) != m_pendingSeeds.end();
    }

    void storePendingSeed(uint8_t level, const ByteArray &seed) {
        m_pendingSeeds[level] = seed;
    }

    void clearPendingSeed(uint8_t level) {
        m_pendingSeeds.erase(level);
    }

    const ByteArray *getPendingSeed(uint8_t level) const {
        auto it = m_pendingSeeds.find(level);
        return it != m_pendingSeeds.end() ? &it->second : nullptr;
    }

    void startSecurityAccessDelay(uint8_t level) {
        m_securityDelayExpiry[level] = std::chrono::steady_clock::now() + getSecurityAccessDelay();
    }

    bool isSecurityAccessDelayExpired(uint8_t level) const {
        auto it = m_securityDelayExpiry.find(level);
        if (it == m_securityDelayExpiry.end()) {
            return true; // No delay active
        }
        return std::chrono::steady_clock::now() >= it->second;
    }


  private:
    DiagnosticSessionControlType m_currentSession = DiagnosticSessionControlType::DefaultSession;
    UdsModelEventHandler m_eventHandler;
    uint16_t m_p2_ms = 2000;
    uint16_t m_p2star_10ms = 400;


    // Security Access State
    std::map<uint8_t, bool> m_unlockedSecurityLevels;                               // level -> unlocked
    std::map<uint8_t, uint8_t> m_securityAttempts;                                  // level -> attempt count
    std::map<uint8_t, ByteArray> m_pendingSeeds;                                    // level -> last sent seed
    std::map<uint8_t, std::chrono::steady_clock::time_point> m_securityDelayExpiry; // level -> delay expiry time
};

using UniqueUdsModelPtr = std::unique_ptr<IUdsModel>;
using SharedUdsModelPtr = std::shared_ptr<IUdsModel>;

} // namespace doip::uds

#endif /* IUDSMODEL_H */
