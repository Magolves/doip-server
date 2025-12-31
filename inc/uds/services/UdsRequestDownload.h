#pragma once

#include "../UdsServiceHandler.h"

namespace doip::uds {

class RequestDownloadHandler : public UdsServiceHandler {
public:
    ~RequestDownloadHandler() override = default;
    ByteArray handle(const ByteArray& request, const UniqueUdsModelPtr& model) override {
        uint32_t memoryAddress = (static_cast<uint32_t>(request[1]) << 24) |
                                 (static_cast<uint32_t>(request[2]) << 16) |
                                 (static_cast<uint32_t>(request[3]) << 8) |
                                 static_cast<uint32_t>(request[4]);

        uint32_t memoryLength = (static_cast<uint32_t>(request[5]) << 24) |
                                (static_cast<uint32_t>(request[6]) << 16) |
                                (static_cast<uint32_t>(request[7]) << 8) |
                                static_cast<uint32_t>(request[8]);

        if (model) {
            UdsResponseCode result = model->requestDownload(memoryAddress, memoryLength, {});
            if (result != UdsResponseCode::PositiveResponse) {
                return makeNegativeResponse(result, request);
            }
        }

        ByteArray responseData;
        responseData.writeU8(sidResponseCode(request));
        responseData.writeU8(0x20);
        responseData.writeU16(MAX_UDS_MESSAGE_LENGTH); // maxNumberOfBlockLength = 4096 bytes
        responseData.writeU32(memoryAddress); // Echo back the memory address
        responseData.writeU32(memoryLength);  // Echo back the memory length

        return responseData;
    }

protected:
    using UdsServiceHandler::makeResponse;
    using UdsServiceHandler::makeNegativeResponse;
};

} // namespace doip::uds
