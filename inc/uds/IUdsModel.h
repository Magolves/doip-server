#ifndef IUDSMODEL_H
#define IUDSMODEL_H

#include "util/ByteArray.h"
#include "uds/UdsTypes.h"
#include "uds/UdsResponseCode.h"
// Removed to avoid circular includes; DiagnosticSessionControlType is in UdsTypes.h

#include <string_view>
#include <functional>
#include <memory>

namespace doip::uds {

enum class UdsModelEvent : uint8_t {
    DiagnosticSessionChange,
    EcuReset,
    SecurityAccessGranted,
    DataByIdentifierRead,
    DataByIdentifierWrite
};

/** Event data passed to UdsModelEventHandler callbacks */
struct IModelEventData {

};


class IUdsModel;

using UdsModelEventHandler = std::function<void(UdsModelEvent event, const IUdsModel& model, IModelEventData const &data)>;

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
     */
    virtual UdsResponseCode setCurrentSession(DiagnosticSessionControlType session) {
        if (m_currentSession == session) {
            return UdsResponseCode::ConditionsNotCorrect;
        }

        m_currentSession = session;
        if (m_eventHandler) {
            m_eventHandler(UdsModelEvent::DiagnosticSessionChange, *this, IModelEventData{});
        }
        return UdsResponseCode::PositiveResponse;
    }

    virtual UdsResponseCode reset(EcuResetType resetType) {
        (void)resetType;
        return UdsResponseCode::PositiveResponse;
    }

    virtual bool supportsDataByIdentifier(uds_did did) const = 0;
    virtual UdsResponseCode getDataByIdentfier(uds_did did, ByteArray &data, size_t offset = 0) const = 0;
    virtual UdsResponseCode setDataByIdentfier(uds_did did, const ByteArray &data, size_t offset = 0) = 0;

    virtual UdsResponseCode startSecurityAccess(uint8_t securityLevel, ByteArray &challenge) {
        (void)securityLevel;
        (void)challenge;
        return UdsResponseCode::ServiceNotSupported;
    }

    virtual UdsResponseCode verifySecurityAccess(uint8_t securityLevel, const ByteArray &key) {
        (void)securityLevel;
        (void)key;
        return UdsResponseCode::ServiceNotSupported;
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

private:
    DiagnosticSessionControlType m_currentSession = DiagnosticSessionControlType::DefaultSession;
    UdsModelEventHandler m_eventHandler;
    uint16_t m_p2_ms = 2000;
    uint16_t m_p2star_10ms = 400;

};

using UniqueUdsModelPtr = std::unique_ptr<IUdsModel>;
using SharedUdsModelPtr = std::shared_ptr<IUdsModel>;

} // namespace doip::uds

#endif /* IUDSMODEL_H */
