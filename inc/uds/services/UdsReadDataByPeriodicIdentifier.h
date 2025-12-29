#pragma once

#include "IUdsServiceHandler.h"
#include "UdsServices.h"

namespace doip::uds {

class ReadDataByPeriodicIdentifierHandler : public IUdsServiceHandler {
public:
    ~ReadDataByPeriodicIdentifierHandler() override = default;
    UdsResponse handle(const ByteArray& request) override;
protected:
    using IUdsServiceHandler::makeResponse;
    using IUdsServiceHandler::makeNegativeResponse;
};

} // namespace doip::uds
