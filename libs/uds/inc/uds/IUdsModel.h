#ifndef IUDSMODEL_H
#define IUDSMODEL_H

#include "uds/UdsDiagnosticTroubleCode.h"
#include "uds/UdsResponseCode.h"
#include "uds/UdsTypes.h"
#include "uds/security/SeedProvider.h"
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

    bool isSecurityLevelUnlocked(uint8_t level) const {
        return m_securityProvider->isSecurityLevelUnlocked(level);
    }

    void lockAllSecurityLevels() {
        m_securityProvider->lockAllSecurityLevels();
    }

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
        // Generate seed (must be overridden by derived class)
        return m_securityProvider->generateSeed(securityLevel, seed);
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
        return m_securityProvider->sendKey(seedLevel, key);

        // // Check if seed was requested first
        // if (!hasPendingSeed(seedLevel)) {
        //     return UdsResponseCode::RequestSequenceError;
        // }

        // // Check attempt counter
        // if (getSecurityAccessAttempts(seedLevel) >= getMaxSecurityAttempts()) {
        //     return UdsResponseCode::ExceededNumberOfAttempts;
        // }

        // // Verify key (must be overridden by derived class)
        // bool valid = verifyKey(seedLevel, key);

        // if (valid) {
        //     unlockSecurityLevel(seedLevel);
        //     resetSecurityAccessAttempts(seedLevel);
        //     clearPendingSeed(seedLevel);

        //     if (m_eventHandler) {
        //         m_eventHandler(UdsModelEvent::SecurityAccessGranted, *this, IModelEventData{});
        //     }

        //     return UdsResponseCode::PositiveResponse;
        // } else {
        //     incrementSecurityAccessAttempts(seedLevel);
        //     clearPendingSeed(seedLevel);

        //     if (m_eventHandler) {
        //         if (getSecurityAccessAttempts(seedLevel) >= getMaxSecurityAttempts()) {
        //             m_eventHandler(UdsModelEvent::SecurityAccessAttemptExceeded, *this, IModelEventData{});
        //             startSecurityAccessDelay(seedLevel);
        //         } else {
        //             m_eventHandler(UdsModelEvent::SecurityAccessDenied, *this, IModelEventData{});
        //         }
        //     }

        //     return UdsResponseCode::InvalidKey;
        // }
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

    virtual DiagnosticTroubleCodeStore &getDTCStore() {
        return m_dtcStore;
    }

  private:
    std::unique_ptr<security::ISecurityAccessProvider> m_securityProvider{std::make_unique<security::SeedProvider>()};
    DiagnosticSessionControlType m_currentSession = DiagnosticSessionControlType::DefaultSession;
    DiagnosticTroubleCodeStore m_dtcStore{};
    UdsModelEventHandler m_eventHandler;
    uint16_t m_p2_ms = 2000;
    uint16_t m_p2star_10ms = 400;
};

using UniqueUdsModelPtr = std::unique_ptr<IUdsModel>;
using SharedUdsModelPtr = std::shared_ptr<IUdsModel>;

} // namespace doip::uds

#endif /* IUDSMODEL_H */
