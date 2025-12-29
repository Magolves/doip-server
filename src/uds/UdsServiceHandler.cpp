#include "uds/UdsServiceHandler.h"
#include "uds/UdsResponseCode.h"
#include "util/ByteArray.h"


namespace doip::uds {

UdsResponse UdsServiceHandler::makeResponse(const ByteArray& /*request*/, const ByteArray& data) {
    return {UdsResponseCode::PositiveResponse, data};
}

UdsResponse UdsServiceHandler::makeNegativeResponse(UdsResponseCode code, const ByteArray& request) const {
    ByteArray negativeResponse;
    negativeResponse.writeU8(0x7F);                                // Negative response indicator
    negativeResponse.writeU8(request.empty() ? 0x00 : request[0]); // Original service ID or 0
    negativeResponse.writeU8(static_cast<uint8_t>(code));          // NRC
    return {code, negativeResponse};
}

} // namespace doip::uds