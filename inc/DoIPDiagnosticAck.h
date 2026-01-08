#ifndef DOIPNEGATIVEDIAGNOSTICACK_H
#define DOIPNEGATIVEDIAGNOSTICACK_H

#include <stdint.h>
#include <optional>
#include <iostream>
#include <iomanip>

namespace doip {

// Table 24/26
enum class DoIPDiagnosticAck : uint8_t {
    PositiveAck = 0, // Table 24
    // 1: reserved
    InvalidSourceAddress = 2,
    UnknownTargetAddress = 3,
    DiagnosticMessageTooLarge = 4,
    OutOfMemory = 5,
    TargetUnreachable = 6, // optional for Table 26
    UnknownNetwork = 7, // optional for Table 26
    TransportProtocolError = 8, //  optional for Table 26, also use if other error codes do not apply
    TargetBusy = 9,  // optional for Table 26
};

/**
 * @brief Stream output operator for DoIPDiagnosticAck
 *
 * @param os the output stream
 * @param ackCode the acknowledgment code
 * @return std::ostream& the output stream
 */
inline std::ostream& operator<<(std::ostream& os, doip::DoIPDiagnosticAck ackCode) {
    const char* name = nullptr;
    switch (ackCode) {
        case doip::DoIPDiagnosticAck::PositiveAck:
            name = "Positive Ack";
            break;
        case doip::DoIPDiagnosticAck::InvalidSourceAddress:
            name = "NACK: Invalid Source Address";
            break;
        case doip::DoIPDiagnosticAck::UnknownTargetAddress:
            name = "NACK: Unknown Target Address";
            break;
        case doip::DoIPDiagnosticAck::DiagnosticMessageTooLarge:
            name = "NACK: Diagnostic Message Too Large";
            break;
        case doip::DoIPDiagnosticAck::OutOfMemory:
            name = "NACK: Out Of Memory";
            break;
        case doip::DoIPDiagnosticAck::TargetUnreachable:
            name = "NACK: Target Unreachable";
            break;
        case doip::DoIPDiagnosticAck::UnknownNetwork:
            name = "NACK: Unknown Network";
            break;
        case doip::DoIPDiagnosticAck::TransportProtocolError:
            name = "NACK: Transport Protocol Error";
            break;
        case doip::DoIPDiagnosticAck::TargetBusy:
            name = "NACK: Target Busy";
            break;
        default:
            name = "Unknown";
            break;
    }
    os << name << " (0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
       << static_cast<unsigned int>(static_cast<uint8_t>(ackCode)) << std::dec << ")";

    return os;
}

}
#endif  /* DOIPNEGATIVEDIAGNOSTICACK_H */
