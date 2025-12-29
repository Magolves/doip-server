#pragma once

#include "../UdsServiceHandler.h"

namespace doip::uds {

class CommunicationControlHandler : public UdsServiceHandler {
public:
    ~CommunicationControlHandler() override = default;
    UdsResponse handle(const ByteArray& request, const UniqueUdsModelPtr& model) override;
protected:
    using UdsServiceHandler::makeResponse;
    using UdsServiceHandler::makeNegativeResponse;
};

} // namespace doip::uds
