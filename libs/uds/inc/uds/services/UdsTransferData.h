#pragma once

#include "../UdsServiceHandler.h"

namespace doip::uds {

class TransferDataHandler : public UdsServiceHandler {
  public:
    ~TransferDataHandler() override = default;
    ByteArray handle(const ByteArray &request, const UniqueUdsModelPtr &model) override {
        uint8_t blockSequenceCounter = request[1];

        if (model) {
            ByteArray data(request, 2, request.size() - 2);
            UdsResponseCode result = model->transferData(blockSequenceCounter, data);
            // in the standard, no negative response is defined for TransferData, see p. 339 of ISO 14229-1:2020
            if (result != UdsResponseCode::PositiveResponse) {
                return makeNegativeResponse(result, request); // although not standard-compliant, we return negative response on error
            }
        }

        return makeResponse(request, {blockSequenceCounter});
    }

  protected:
    using UdsServiceHandler::makeNegativeResponse;
    using UdsServiceHandler::makeResponse;
};

} // namespace doip::uds
