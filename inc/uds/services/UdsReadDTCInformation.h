#pragma once

#include "../UdsServiceHandler.h"

namespace doip::uds {

class ReadDTCInformationHandler : public UdsServiceHandler {
public:
    ~ReadDTCInformationHandler() override = default;
    UdsResponse handle(const ByteArray& request, const UniqueUdsModelPtr& model) override;
protected:
    using UdsServiceHandler::makeResponse;
    using UdsServiceHandler::makeNegativeResponse;
};

} // namespace doip::uds
