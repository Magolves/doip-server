#pragma once

#include "../UdsServiceHandler.h"

namespace doip::uds {

class SecurityAccessHandler : public UdsServiceHandler {
public:
    ~SecurityAccessHandler() override = default;
    ByteArray handle(const ByteArray& request, const UniqueUdsModelPtr& model) override;

protected:
    using UdsServiceHandler::makeResponse;
    using UdsServiceHandler::makeNegativeResponse;

    /**
     * @brief Handle requestSeed sub-function (odd security levels: 0x01, 0x03, 0x05, ...)
     */
    ByteArray handleRequestSeed(const ByteArray& request, const UniqueUdsModelPtr& model, uint8_t securityLevel);

    /**
     * @brief Handle sendKey sub-function (even security levels: 0x02, 0x04, 0x06, ...)
     */
    ByteArray handleSendKey(const ByteArray& request, const UniqueUdsModelPtr& model, uint8_t securityLevel);
};

} // namespace doip::uds
