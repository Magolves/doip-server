#ifndef UDSTYPES_H
#define UDSTYPES_H

#include <cstdint>

namespace doip::uds {
using uds_length = uint16_t;

/**
 * @brief UDS Service Identifier (SID) type.
 */
using uds_sid = uint8_t;

/**
 * @brief UDS Data Identifier (DID) type.
 */
using uds_did = uint16_t;

/**
 * @brief UDS Response Code type.
 */
using uds_rsp_code = uint8_t;

// Maximum UDS message length (ISO-14229)
constexpr uds_length MAX_UDS_MESSAGE_LENGTH = 4095;

/**
 * @brief Diagnostic Session Control Types (SID 0x10).
 */
enum class DiagnosticSessionControlType : uint8_t {
    DefaultSession = 0x01,
    ProgrammingSession = 0x02,
    ExtendedDiagnosticSession = 0x03,
    SafetySystemDiagnosticSession = 0x04
};

enum class EcuResetType : uint8_t {
    HardReset = 0x01,
    KeyOffOnReset = 0x02,
    SoftReset = 0x03
};

} // namespace doip::uds

#endif /* UDSTYPES_H */
