#pragma once

#include "../UdsServiceHandler.h"

namespace doip::uds {

class WriteDataByIdentifierHandler : public UdsServiceHandler {
public:
    ~WriteDataByIdentifierHandler() override = default;
    ByteArray handle(const ByteArray& request, const UniqueUdsModelPtr& model) override {
        uint16_t did = (static_cast<uint16_t>(request[1]) << 8) | request[2];

        if (model) {
            UdsResponseCode result = model->setDataByIdentfier(did, request, 3);
            if (result != UdsResponseCode::PositiveResponse) {
                return makeNegativeResponse(result, request);
            }
        }

        return makeDidResponse(request, {});
    }
protected:
    using UdsServiceHandler::makeResponse;
    using UdsServiceHandler::makeNegativeResponse;
};

} // namespace doip::uds
