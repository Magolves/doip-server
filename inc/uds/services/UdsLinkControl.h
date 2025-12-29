#pragma once

#include "IUdsServiceHandler.h"
#include "UdsServices.h"

namespace doip::uds {

class LinkControlHandler : public IUdsServiceHandler {
public:
    ~LinkControlHandler() override = default;
    UdsResponse handle(const ByteArray& request) override;
protected:
    using IUdsServiceHandler::makeResponse;
    using IUdsServiceHandler::makeNegativeResponse;
};

} // namespace doip::uds
