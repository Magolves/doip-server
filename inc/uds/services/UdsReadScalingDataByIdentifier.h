#pragma once

#include "IUdsServiceHandler.h"
#include "UdsServices.h"

namespace doip::uds {

class ReadScalingDataByIdentifierHandler : public IUdsServiceHandler {
public:
    ~ReadScalingDataByIdentifierHandler() override = default;
    UdsResponse handle(const ByteArray& request) override;
protected:
    using IUdsServiceHandler::makeResponse;
    using IUdsServiceHandler::makeNegativeResponse;
};

} // namespace doip::uds
