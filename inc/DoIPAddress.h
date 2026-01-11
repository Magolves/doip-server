#ifndef DOIPADDRESS_H
#define DOIPADDRESS_H

#include <stddef.h>
#include <stdint.h>

#include <iomanip>
#include <iostream>
#include <sstream>

namespace doip {

constexpr size_t DOIP_ADDRESS_SIZE = 2;

/**
 * @brief Represents a 16-bit DoIP value consisting of high and low significant bytes.
 *
 * This structure encapsulates a 16-bit value used in DoIP (Diagnostic over Internet Protocol)
 * communication. The value is stored as two separate bytes (HSB and LSB) and provides
 * convenient methods for construction, access, and manipulation.
 *
 * @note The value follows big-endian byte ordering (HSB first, then LSB).
 */
using DoIPAddress = uint16_t;

constexpr DoIPAddress ZERO_ADDRESS = 0x0000;
constexpr DoIPAddress MIN_SOURCE_ADDRESS = 0xE000;
constexpr DoIPAddress MAX_SOURCE_ADDRESS = 0xE3FF;

/**
 * @brief Check if source value is valid.
 * @param data the data array containing the value
 * @param offset the offset in the data array where the value starts
 * @return true the source value is valid
 * @return false the source value is NOT valid
 */
inline bool isValidSourceAddress(const uint8_t *data, size_t offset = 0) noexcept {
    uint16_t addr_value = (data[offset] << 8) | data[offset + 1];

    return MIN_SOURCE_ADDRESS <= addr_value && MAX_SOURCE_ADDRESS >= addr_value;
}

/**
 * @brief Try read the DoIP value from a byte array.
 * @note No bounds checking
 * @param data the pointer to the data array
 * @param offset the offset in bytes
 * @param value the value read
 * @return true value was read successfully
 * @return false invalid arguments
 */
inline bool tryReadAddressFrom(const uint8_t *data, size_t offset, DoIPAddress &value) {
    if (!data)
        return false;

    value = static_cast<uint16_t>(data[offset] << 8 | data[offset + 1]);
    return true;
}

/**
 * @brief Reads the DoIP value from a byte array.
 * @note No bounds checking
 * @param data the pointer to the data array
 * @param offset the offset in bytes
 * @return DoIPAddress the parsed DoIP value. If data is nullptr, then ZERO_ADDRESS is returned.
 */
inline DoIPAddress readAddressFrom(const uint8_t *data, size_t offset = 0) {
    if (!data)
        return ZERO_ADDRESS;

    return static_cast<uint16_t>(data[offset] << 8 | data[offset + 1]);
}

template <typename T>
inline std::string toHex4 (T value) {
    static_assert(sizeof(T) == 2, "toHex4 requires a 16-bit type");
    std::ostringstream os;
    os << "0x"
    << std::hex
    << std::nouppercase
    << std::setw(4)
    << std::setfill('0')
    << static_cast<uint16_t>(value)
    << std::dec;
    return os.str();
}

} // namespace doip

#endif /* DOIPADDRESS_H */
