#ifndef IUDSSERVICEHANDLER_H
#define IUDSSERVICEHANDLER_H

#include "IUdsModel.h"
#include "UdsResponseCode.h"
#include "UdsServices.h"
#include "UdsTypes.h"
#include "util/ByteArray.h"
#include <memory>

namespace doip::uds {

using UdsResponse = std::pair<UdsResponseCode, ByteArray>;

inline std::ostream &operator<<(std::ostream &os, const UdsResponse &response) {
    std::ios_base::fmtflags flags(os.flags());

    os << response.first << " [";
    os << std::hex << std::uppercase << std::setw(2) << std::setfill('0');

    for (size_t i = 0; i < response.second.size(); ++i) {
        if (i > 0) {
            os << '.';
        }
        os << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
           << static_cast<unsigned int>(response.second[i]);
    }

    os.flags(flags);
    return os;
}

class UdsServiceHandler {
  public:
    virtual ~UdsServiceHandler() = default;
    virtual ByteArray handle(const ByteArray &request, const UniqueUdsModelPtr &model) = 0;

  protected:
    /**
     * @brief Check if request has minimum required length
     */
    static bool checkMinLength(const ByteArray &request, size_t minLength) {
        return request.size() >= minLength;
    }

    /**
     * @brief Extract uint16_t value from request at given offset (big-endian)
     */
    static uint16_t extractU16(const ByteArray &request, size_t offset) {
        if (offset + 1 >= request.size()) return 0;
        return (static_cast<uint16_t>(request[offset]) << 8) | request[offset + 1];
    }

    /**
     * @brief Extract uint32_t value from request at given offset (big-endian)
     */
    static uint32_t extractU32(const ByteArray &request, size_t offset) {
        if (offset + 3 >= request.size()) return 0;
        return (static_cast<uint32_t>(request[offset]) << 24) |
               (static_cast<uint32_t>(request[offset + 1]) << 16) |
               (static_cast<uint32_t>(request[offset + 2]) << 8) |
               static_cast<uint32_t>(request[offset + 3]);
    }

  public:
    static ByteArray makeResponse(const ByteArray &request, const ByteArray &data = {}) {
        ByteArray positiveResponse;
        positiveResponse.writeU8(sidResponseCode(request.empty() ? 0x00 : request[0]));
        positiveResponse.insert(positiveResponse.end(), data.begin(), data.end());
        return positiveResponse;
    }

    static ByteArray makeDidResponse(const ByteArray &request, const ByteArray &data = {}) {
        ByteArray positiveResponse;
        positiveResponse.writeU8(sidResponseCode(request.empty() ? 0x00 : request[0]));
        positiveResponse.writeU8(request[1]); // DID high byte
        positiveResponse.writeU8(request[2]); // DID low byte

        positiveResponse.insert(positiveResponse.end(), data.begin(), data.end());
        return positiveResponse;
    }

    static ByteArray makeNegativeResponse(UdsResponseCode code, const ByteArray &request) {
        ByteArray negativeResponse;
        negativeResponse.writeU8(UDS_NEGATIVE_RESPONSE_INDICATOR);                                // Negative response indicator
        negativeResponse.writeU8(request.empty() ? 0x00 : request[0]); // Original service ID or 0
        negativeResponse.writeU8(static_cast<uint8_t>(code));          // NRC
        return negativeResponse;
    }
};

using UniqueUdsServiceHandlerPtr = std::unique_ptr<UdsServiceHandler>;
using IUdsServiceHandlerPtr = std::unique_ptr<UdsServiceHandler>;

} // namespace doip::uds

#endif // IUDSSERVICEHANDLER_H
