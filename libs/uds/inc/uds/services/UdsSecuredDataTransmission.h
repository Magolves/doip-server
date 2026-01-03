#pragma once

#include "../UdsServiceHandler.h"

namespace doip::uds {

class SecuredDataTransmissionHandler : public UdsServiceHandler {
public:
    ~SecuredDataTransmissionHandler() override = default;
    ByteArray handle(const ByteArray& request, const UniqueUdsModelPtr& model) override;
protected:
    using UdsServiceHandler::makeResponse;
    using UdsServiceHandler::makeNegativeResponse;
};

} // namespace doip::uds
