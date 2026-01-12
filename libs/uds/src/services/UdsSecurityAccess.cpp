#include "uds/services/UdsSecurityAccess.h"
#include "uds/IUdsModel.h"
#include "uds/UdsResponseCode.h"

namespace doip::uds {

ByteArray SecurityAccessHandler::handle(const ByteArray& request, const UniqueUdsModelPtr& model) {
    if (!model) {
        return makeNegativeResponse(UdsResponseCode::GeneralProgrammingFailure, request);
    }

    uint8_t subFunction = request[1];

    // Security Access sub-functions:
    // Odd (0x01, 0x03, 0x05, ...): requestSeed
    // Even (0x02, 0x04, 0x06, ...): verifyKey

    if (subFunction == 0x00 || subFunction > 0x7F) {
        // Sub-function out of range (0x00 reserved, > 0x7F invalid)
        return makeNegativeResponse(UdsResponseCode::SubFunctionNotSupported, request);
    }

    if (subFunction % 2 == 1) {
        // Odd: requestSeed
        std::cout << "Handling requestSeed for security level " << static_cast<int>(subFunction) << "\n";
        return handleRequestSeed(request, model, subFunction);
    } else {
        // Even: verifyKey
        std::cout << "Handling verifyKey for security level " << static_cast<int>(subFunction) << "\n";
        return handleSendKey(request, model, subFunction);
    }
}

ByteArray SecurityAccessHandler::handleRequestSeed(
    const ByteArray& request,
    const UniqueUdsModelPtr& model,
    uint8_t securityLevel) {

    // requestSeed: SID (1) + sub-function (1) = 2 bytes
    // Optional: security access data record (manufacturer specific)
    if (request.size() > 2) {
        // Some OEMs require additional data (e.g., VIN, programming date)
        // For now, we ignore additional data
    }

    ByteArray seed;
    UdsResponseCode result = model->requestSeed(securityLevel, seed);

    std::cout << "Handling requestSeed for security level " << +static_cast<int>(securityLevel) << " returned  " << result << "\n";

    if (result != UdsResponseCode::PositiveResponse) {
        return makeNegativeResponse(result, request);
    }

    // Build positive response: SID+0x40, sub-function, seed bytes
    ByteArray response;
    response.push_back(sidResponseCode(request[0]));
    response.push_back(securityLevel);
    response.insert(response.end(), seed.begin(), seed.end());

    return response;
}

ByteArray SecurityAccessHandler::handleSendKey(
    const ByteArray& request,
    const UniqueUdsModelPtr& model,
    uint8_t securityLevel) {

    // verifyKey: SID (1) + sub-function (1) + key bytes (2-255)
    if (request.size() < 3) {
        return makeNegativeResponse(UdsResponseCode::IncorrectMessageLengthOrInvalidFormat, request);
    }

    // Extract key from request (everything after SID + sub-function)
    ByteArray key(request, 2, request.size() - 2);

    UdsResponseCode result = model->verifyKey(securityLevel, key);

    std::cout << "Handling verifyKey for security level " << +static_cast<int>(securityLevel) << " returned  " << result << "\n";

    if (result != UdsResponseCode::PositiveResponse) {
        return makeNegativeResponse(result, request);
    }

    ByteArray response;
    response.writeU8(sidResponseCode(request[0]));
    response.writeU8(securityLevel);

    return response;
}

} // namespace doip::uds
