#pragma once

#include "IUdsServiceHandler.h"
#include "UdsServices.h"

namespace doip::uds {

class ECUResetHandler : public IUdsServiceHandler {
public:
    ~ECUResetHandler() override = default;
    UdsResponse handle(const ByteArray& request) override;
protected:
    using IUdsServiceHandler::makeResponse;
    using IUdsServiceHandler::makeNegativeResponse;
};

} // namespace doip::uds
