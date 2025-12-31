#pragma once

#include "../UdsServiceHandler.h"

namespace doip::uds {

class TransferDataHandler : public UdsServiceHandler {
public:
    ~TransferDataHandler() override = default;
    ByteArray handle(const ByteArray& request, const UniqueUdsModelPtr& model) override {

        if (model) {
            uint8_t blockSequenceCounter = request[1];
            ByteArray data(request, 2, request.size() - 2);
            UdsResponseCode result = model->transferData(blockSequenceCounter, data);
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
