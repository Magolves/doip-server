#pragma once

#include "../UdsServiceHandler.h"

namespace doip::uds {

class RequestDownloadHandler : public UdsServiceHandler {
  public:
    ~RequestDownloadHandler() override = default;
    ByteArray handle(const ByteArray &request, const UniqueUdsModelPtr &model) override {
        // RequestDownload request format:
        // Byte 0: Service ID (0x34)
        // Byte 1: Data format Identifier (usually 0x00)
        // Byte 2: Address and Length Format Identifier (e.g., 0x20 for 4-byte address and 4-byte length)
        // Byte 3..n: Memory Address (1-4 bytes)
        // Byte n+1..m: Memory Length (1-4 bytes)
        // 3400144000000064 -> 1 bytes address, 4 bytes length, address=0x00, length=100


        if (request[1] != 0x00) {
            return makeNegativeResponse(UdsResponseCode::SubFunctionNotSupported, request);
        }

        uint8_t alf = request[2]; // addressAndLengthFormatIdentifier, usually 0x20 for 4-byte address and 4-byte length

        uint8_t lengthLength = lowNibble(alf);   // number of bytes for length
        uint8_t addressLength = highNibble(alf); // number of bytes for address

        uint32_t memoryAddress = 0L;
        uint32_t memoryLength = 0L;

        if (request.size() < static_cast<size_t>(3 + addressLength + lengthLength)) {
            return makeNegativeResponse(UdsResponseCode::IncorrectMessageLengthOrInvalidFormat, request);
        }

        for (uint8_t i = 0; i < addressLength; ++i) {
            memoryAddress = (memoryAddress << 8) | request[3 + i];
        }

        for (uint8_t i = 0; i < lengthLength; ++i) {
            memoryLength = (memoryLength << 8) | request[static_cast<size_t>(3 + addressLength + i)];
        }

        if (model) {
            UdsResponseCode result = model->requestDownload(memoryAddress, memoryLength, {});
            if (result != UdsResponseCode::PositiveResponse) {
                return makeNegativeResponse(result, request);
            }
        }

        m_logger->info("RequestDownload: address=0x{:08X}, length={}", memoryAddress, memoryLength);

        ByteArray responseData;
        responseData.writeU8(sidResponseCode(request));
        responseData.writeU8(0x20); // currently only 4-byte maxNumberOfBlockLength supported
        responseData.writeU16(MAX_UDS_MESSAGE_LENGTH); // maxNumberOfBlockLength = 4095 bytes

        return responseData;
    }

  protected:
    using UdsServiceHandler::makeNegativeResponse;
    using UdsServiceHandler::makeResponse;
};

} // namespace doip::uds
