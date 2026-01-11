#pragma once

#include "../UdsServiceHandler.h"

namespace doip::uds {

class ReadDTCInformationHandler : public UdsServiceHandler {
public:
    ~ReadDTCInformationHandler() override = default;
    ByteArray handle(const ByteArray& request, const UniqueUdsModelPtr& model) override {
        uint8_t subFunction = request[1];

        ByteArray responseData;
        responseData.writeU8(sidResponseCode(request[0]));
        responseData.writeU8(subFunction);

        auto dtcStore = model.get()->getDTCStore();

        // For simplicity, we return a fixed number of DTCs
        // In a real implementation, this would query the model for actual DTCs
        dtcStore.findDTC

        // Dummy DTCs
        for (uint8_t i = 0; i < numberOfDTCs; ++i) {
            responseData.writeU24(0x123456 + i); // DTC code
            responseData.writeU8(0xFF);          // DTC status
        }

        return responseData;
    }
protected:
    using UdsServiceHandler::makeResponse;
    using UdsServiceHandler::makeNegativeResponse;
};

} // namespace doip::uds
