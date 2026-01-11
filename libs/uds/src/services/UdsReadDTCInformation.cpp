#include "uds/services/UdsReadDTCInformation.h"
#include "uds/UdsDiagnosticTroubleCode.h"

namespace doip::uds {

UdsResponseCode handleReportNumberOfDTCByStatusMask(const ByteArray& request, const DiagnosticTroubleCodeStore& store, ByteArray& responseData) {
    // table 302/319: Byte 2 - DTC Status Mask (DTCSM)
    uint8_t statusMask = request[2];

    responseData.writeU8(0xff); // complete status mask ???
    responseData.writeU8(DTC_FORMAT_IDENTIFIER); // format identifier (??)

    size_t numberOfDTCs = store.countByStatusBits(statusMask);

    responseData.writeU16(static_cast<uint16_t>(numberOfDTCs));

    return UdsResponseCode::PositiveResponse;
}

UdsResponseCode handleReportDTCByStatusMask(const ByteArray& request, const DiagnosticTroubleCodeStore& store, ByteArray& responseData) {
    // table 302/319: Byte 2 - DTC Status Mask (DTCSM)
    uint8_t statusMask = request[2];

    responseData.writeU8(0xff); // complete status mask

    store.findDTC(statusMask);

    return UdsResponseCode::PositiveResponse;
}

} // namespace doip::uds