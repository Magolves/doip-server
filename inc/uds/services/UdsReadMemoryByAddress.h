#pragma once

#include "../UdsServiceHandler.h"

namespace doip::uds {

class ReadMemoryByAddressHandler : public UdsServiceHandler {
public:
    ~ReadMemoryByAddressHandler() override = default;
    ByteArray handle(const ByteArray& request, const UniqueUdsModelPtr& model) override;
protected:
    using UdsServiceHandler::makeResponse;
    using UdsServiceHandler::makeNegativeResponse;
};

} // namespace doip::uds
