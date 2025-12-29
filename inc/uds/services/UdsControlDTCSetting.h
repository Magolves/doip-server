#pragma once

#include "../UdsServiceHandler.h"

namespace doip::uds {

class ControlDTCSettingHandler : public UdsServiceHandler {
public:
    ~ControlDTCSettingHandler() override = default;
    UdsResponse handle(const ByteArray& request, const UniqueUdsModelPtr& model) override;
protected:
    using UdsServiceHandler::makeResponse;
    using UdsServiceHandler::makeNegativeResponse;
};

} // namespace doip::uds
