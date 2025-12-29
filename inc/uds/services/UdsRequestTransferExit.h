#pragma once

#include "../UdsServiceHandler.h"

namespace doip::uds {

class RequestTransferExitHandler : public UdsServiceHandler {
public:
    ~RequestTransferExitHandler() override = default;
    UdsResponse handle(const ByteArray& request, const UniqueUdsModelPtr& model) override {
        (void)request;
        if (model) {
            UdsResponseCode result = model->requestTransferExit();
            if (result != UdsResponseCode::PositiveResponse) {
                return makeNegativeResponse(result, request);
            }
        }

        return makeResponse(request, {});
    }

protected:
    using UdsServiceHandler::makeResponse;
    using UdsServiceHandler::makeNegativeResponse;
};

} // namespace doip::uds
