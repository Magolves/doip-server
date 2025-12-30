#pragma once

#include "../UdsServiceHandler.h"

namespace doip::uds {

class ECUResetHandler : public UdsServiceHandler {
public:
    ~ECUResetHandler() override = default;
    ByteArray handle(const ByteArray& request, const UniqueUdsModelPtr& model) override {
        uint8_t resetType = request[1];


        if (model) {
            UdsResponseCode result = model->reset(static_cast<EcuResetType>(resetType));
            if (result != UdsResponseCode::PositiveResponse) {
                return makeNegativeResponse(result, request);
            }
        }

        return makeResponse(request, {resetType});
    }
protected:
    using UdsServiceHandler::makeResponse;
    using UdsServiceHandler::makeNegativeResponse;
};

} // namespace doip::uds
