#ifndef UDSSERVICES_H
#define UDSSERVICES_H

#include "UdsTypes.h"

#include <array>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace doip::uds {

enum class UdsService : uint8_t {
    None = 0x00,
    DiagnosticSessionControl = 0x10,
    ECUReset = 0x11,
    SecurityAccess = 0x27,
    CommunicationControl = 0x28,
    TesterPresent = 0x3E,
    AccessTimingParameters = 0x83,
    SecuredDataTransmission = 0x84,
    ControlDTCSetting = 0x85,
    ResponseOnEvent = 0x86,
    LinkControl = 0x87,
    ReadDataByIdentifier = 0x22,
    ReadMemoryByAddress = 0x23,
    ReadScalingDataByIdentifier = 0x24,
    ReadDataByPeriodicIdentifier = 0x2A,
    DynamicallyDefineDataIdentifier = 0x2C,
    WriteDataByIdentifier = 0x2E,
    WriteMemoryByAddress = 0x3D,
    ClearDiagnosticInformation = 0x14,
    ReadDTCInformation = 0x19,
    RequestDownload = 0x34,
    TransferData = 0x36,
    RequestTransferExit = 0x37,
};

struct UdsServiceDescriptor {
    UdsService service;
    uds_length minReqLength;
    uds_length maxReqLength;
    uds_length minRspLength;
    uds_length maxRspLength;
};

constexpr std::array<UdsServiceDescriptor, 22> UDS_SERVICE_DESCRIPTORS = {{{UdsService::DiagnosticSessionControl, 2, 2, 6, 6},
                                                                           {UdsService::ECUReset, 2, 2, 2, 2},
                                                                           {UdsService::SecurityAccess, 2, MAX_UDS_MESSAGE_LENGTH, 3, MAX_UDS_MESSAGE_LENGTH},
                                                                           {UdsService::CommunicationControl, 2, MAX_UDS_MESSAGE_LENGTH, 3, MAX_UDS_MESSAGE_LENGTH},
                                                                           {UdsService::TesterPresent, 2, 2, 2, 2},
                                                                           {UdsService::AccessTimingParameters, 2, MAX_UDS_MESSAGE_LENGTH, 3, MAX_UDS_MESSAGE_LENGTH},
                                                                           {UdsService::SecuredDataTransmission, 2, MAX_UDS_MESSAGE_LENGTH, 3, MAX_UDS_MESSAGE_LENGTH},
                                                                           {UdsService::ControlDTCSetting, 2, MAX_UDS_MESSAGE_LENGTH, 3, MAX_UDS_MESSAGE_LENGTH},
                                                                           {UdsService::ResponseOnEvent, 2, MAX_UDS_MESSAGE_LENGTH, 3, MAX_UDS_MESSAGE_LENGTH},
                                                                           {UdsService::LinkControl, 2, MAX_UDS_MESSAGE_LENGTH, 3, MAX_UDS_MESSAGE_LENGTH},
                                                                           {UdsService::ReadDataByIdentifier, 3, MAX_UDS_MESSAGE_LENGTH, 4, MAX_UDS_MESSAGE_LENGTH},
                                                                           {UdsService::ReadMemoryByAddress, 4, MAX_UDS_MESSAGE_LENGTH, 4, MAX_UDS_MESSAGE_LENGTH},
                                                                           {UdsService::ReadScalingDataByIdentifier, 3, MAX_UDS_MESSAGE_LENGTH, 3, MAX_UDS_MESSAGE_LENGTH},
                                                                           {UdsService::ReadDataByPeriodicIdentifier, 3, MAX_UDS_MESSAGE_LENGTH, 3, MAX_UDS_MESSAGE_LENGTH},
                                                                           {UdsService::DynamicallyDefineDataIdentifier, 3, MAX_UDS_MESSAGE_LENGTH, 3, MAX_UDS_MESSAGE_LENGTH},
                                                                           {UdsService::WriteDataByIdentifier, 4, MAX_UDS_MESSAGE_LENGTH, 3, MAX_UDS_MESSAGE_LENGTH},
                                                                           {UdsService::WriteMemoryByAddress, 4, MAX_UDS_MESSAGE_LENGTH, 3, MAX_UDS_MESSAGE_LENGTH},
                                                                           {UdsService::ClearDiagnosticInformation, 3, MAX_UDS_MESSAGE_LENGTH, 3, MAX_UDS_MESSAGE_LENGTH},
                                                                           {UdsService::ReadDTCInformation, 2, MAX_UDS_MESSAGE_LENGTH, 3, MAX_UDS_MESSAGE_LENGTH},
                                                                           {UdsService::RequestDownload, 6, MAX_UDS_MESSAGE_LENGTH, 3, 6},
                                                                           {UdsService::TransferData, 3, MAX_UDS_MESSAGE_LENGTH, 2, MAX_UDS_MESSAGE_LENGTH},
                                                                           {UdsService::RequestTransferExit, 1, MAX_UDS_MESSAGE_LENGTH, 1, MAX_UDS_MESSAGE_LENGTH}}};

/**
 * @brief Find service descriptor by service ID
 *
 * @param sid the UDS service ID
 * @return const UdsServiceDescriptor* the service descriptor or nullptr if not found
 */
template <typename T = UdsResponseCode>
inline const UdsServiceDescriptor *findServiceDescriptor(T sid) {
    static_assert(std::is_enum_v<T> || std::is_integral_v<T>, "T must be an enum or integral type");
    auto it = std::find_if(UDS_SERVICE_DESCRIPTORS.begin(), UDS_SERVICE_DESCRIPTORS.end(),
                           [sid](const UdsServiceDescriptor &desc) {
                               return static_cast<uint8_t>(desc.service) == static_cast<uint8_t>(sid);
                           });
    if (it != UDS_SERVICE_DESCRIPTORS.end()) {
        return &(*it);
    }
    return nullptr;
}

/**
 * @brief Get the SID Response as byte, e. g. sid 0x22 -> 0x62.
 *
 * @tparam T the type of the response code (enum or integral)
 * @param code the response code
 * @return uint8_t
 */
template <typename T = UdsResponseCode>
inline uint8_t sidResponseCode(const T &code) {
    static_assert(std::is_enum_v<T> || std::is_integral_v<T>, "T must be an enum or integral type");
    return static_cast<uint8_t>(code) | 0x40;
}

inline uint8_t sidResponseCode(const ByteArray &request) {
    if (request.empty()) {
        return 0x00;
    }
    return sidResponseCode(request[0]);
}


} // namespace doip::uds

#endif /* UDSSERVICES_H */
