#ifndef UdsdataidentifiersH
#define UdsdataidentifiersH

#include <cstdint>

namespace doip::uds {

enum class UdsDataIdentifier : uint16_t {
    // DID (Hex),Description
    VIN = 0xF190, // Vehicle Identification Number
    SS_Identifier = 0xF18A,                 // System Supplier Identifier
};


} // namespace doip::uds
#endif /* UdsdataidentifiersH */
