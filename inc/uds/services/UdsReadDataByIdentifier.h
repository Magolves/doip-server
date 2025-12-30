#pragma once

#include <unordered_map>
#include <cstdint>

#include "../UdsServiceHandler.h"
#include "../UdsServices.h"

namespace doip::uds {

class ReadDataByIdentifierHandler : public UdsServiceHandler {
public:
    ~ReadDataByIdentifierHandler() override = default;
    ByteArray handle(const ByteArray& request, const UniqueUdsModelPtr& model) override {
        ByteArray responseData;
        for (size_t i = 1; i < request.size(); i += 2) {
            uds_did did = static_cast<uds_did>((request[i] << 8) | request[i + 1]);
            if (model && model->supportsDataByIdentifier(did)) {
                responseData.writeU8(sidResponseCode(request[0]));
                UdsResponseCode result = model->getDataByIdentfier(did, responseData);
                if (result != UdsResponseCode::PositiveResponse) {
                    return makeNegativeResponse(result, request);
                }
            } else {
                return makeNegativeResponse(UdsResponseCode::RequestOutOfRange, request);
            }
        }

        return responseData;
    }
};

} // namespace doip::uds
