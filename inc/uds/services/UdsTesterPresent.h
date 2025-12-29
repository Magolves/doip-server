#pragma once

#include "../UdsServiceHandler.h"

namespace doip::uds {

class TesterPresentHandler : public UdsServiceHandler {
public:
    ~TesterPresentHandler() override = default;
    UdsResponse handle(const ByteArray& request, const UniqueUdsModelPtr& model) override {
        (void)model;
        // According to ISO 14229-1, Tester Present does not require any action other than responding positively
        return makeResponse(request, {});
    }
protected:
    using UdsServiceHandler::makeResponse;
    using UdsServiceHandler::makeNegativeResponse;
};

} // namespace doip::uds
