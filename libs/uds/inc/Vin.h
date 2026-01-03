#ifndef VIN_H
#define VIN_H

#include "GenericFixedId.h"
#include "util/ByteArray.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <iterator>
#include <sstream>
#include <string>

namespace doip {
/**
 * @brief Vehicle Identification Number (VIN) - 17 bytes according to ISO 3779
 *
 * This class enforces VIN validation rules:
 * - Exactly 17 characters
 * - Only uppercase alphanumeric characters (excluding I, O, Q)
 * - Automatically converts lowercase to uppercase
 * - Padded with ASCII '0' characters when shorter than 17 bytes
 */
class Vin {
  private:
    GenericFixedId<17, true, '0'> m_data;

    /**
     * @brief Check if a character is valid for VINs (excluding I, O, Q)
     */
    static bool isValidVinChar(char c) {
        return (c >= 'A' && c <= 'Z' && c != 'I' && c != 'O' && c != 'Q') ||
               (c >= '0' && c <= '9');
    }

    /**
     * @brief Normalize VIN string: convert to uppercase
     *
     * @param vin Input VIN string
     * @param validate If true, throws on invalid characters
     * @return Normalized VIN string (uppercase)
     * @throws std::invalid_argument if validate=true and VIN contains invalid characters
     */
    static std::string normalizeVin(std::string vin, bool validate = true) {
        // Convert to uppercase
        std::transform(vin.begin(), vin.end(), vin.begin(),
                       [](unsigned char c) { return std::toupper(c); });

        if (validate) {
            // Validate characters
            if (!std::all_of(vin.begin(), vin.end(),
                             [](char c) { return isValidVinChar(c) || c == '0'; })) {
                return "00000000000000000";
            }
        }
        return vin;
    }

  public:
    /// Length of VIN in bytes (ISO 3779)
    static constexpr size_t VIN_LENGTH = 17;

    /// Static zero-initialized VIN instance
    static const Vin Zero;

    /**
     * @brief Default constructor - creates VIN filled with '0' padding
     */
    Vin() : m_data() {}

    /**
     * @brief Construct from string with automatic uppercase conversion
     *
     * @param vin VIN as string. Converted to uppercase and padded/truncated to 17 bytes.
     *            Does NOT validate character set to maintain backward compatibility.
     *            Use isValid() to check validity after construction.
     */
    explicit Vin(const std::string &vin)
        : m_data(normalizeVin(vin, false)) {}

    /**
     * @brief Construct from C-style string with automatic uppercase conversion
     *
     * @param vin VIN as C-string. Converted to uppercase and padded/truncated to 17 bytes.
     *            Does NOT validate character set to maintain backward compatibility.
     *            Use isValid() to check validity after construction.
     */
    explicit Vin(const char *vin)
        : m_data(vin ? normalizeVin(std::string(vin), false) : std::string()) {} /**
                                                                                  * @brief Construct from byte sequence (no validation, assumes pre-validated data)
                                                                                  *
                                                                                  * @param data Pointer to byte data
                                                                                  * @param length Length of byte data
                                                                                  */
    Vin(const uint8_t *data, size_t length)
        : m_data(data, length) {}

    /**
     * @brief Construct from ByteArray (no validation, assumes pre-validated data)
     *
     * @param byte_array VIN as ByteArray
     */
    explicit Vin(const ByteArray &byte_array)
        : m_data(byte_array) {}

    /**
     * @brief Construct from integral value (for numeric VINs)
     *
     * @tparam Integral Integral type
     * @param id_value Numeric VIN value
     */
    template <typename Integral,
              typename = std::enable_if_t<std::is_integral_v<Integral>>>
    explicit Vin(Integral id_value)
        : m_data(id_value) {}

    // Default copy/move semantics
    Vin(const Vin &) = default;
    Vin &operator=(const Vin &) = default;
    Vin(Vin &&) noexcept = default;
    Vin &operator=(Vin &&) noexcept = default;
    ~Vin() = default;

    // Delegate to underlying GenericFixedId
    std::string toString() const {
        return m_data.toString();
    }
    std::string toHexString() const {
        return m_data.toHexString();
    }
    ByteArrayRef asByteArray() const {
        return m_data.asByteArray();
    }
    const std::array<uint8_t, 17> &getArray() const {
        return m_data.getArray();
    }
    const uint8_t *data() const {
        return m_data.data();
    }
    ByteArray &appendTo(ByteArray &bytes) const {
        return m_data.appendTo(bytes);
    }
    constexpr size_t size() const {
        return VIN_LENGTH;
    }
    bool isEmpty() const noexcept {
        return m_data.isEmpty();
    }

    bool operator==(const Vin &other) const noexcept {
        return m_data == other.m_data;
    }
    bool operator!=(const Vin &other) const noexcept {
        return m_data != other.m_data;
    }
    const uint8_t &operator[](size_t index) const {
        return m_data[index];
    }

    constexpr char getPadChar() const noexcept {
        return '0';
    }
    constexpr uint8_t getPadByte() const noexcept {
        return static_cast<uint8_t>('0');
    }

    // Iterator support
    auto begin() noexcept {
        return m_data.begin();
    }
    auto begin() const noexcept {
        return m_data.begin();
    }
    auto cbegin() const noexcept {
        return m_data.cbegin();
    }
    auto end() noexcept {
        return m_data.end();
    }
    auto end() const noexcept {
        return m_data.end();
    }
    auto cend() const noexcept {
        return m_data.cend();
    }

    /**
     * @brief Validate VIN according to ISO 3779 character set
     *
     * @return true if all characters are valid (A-Z except I,O,Q and 0-9)
     */
    bool isValid() const noexcept {
        return std::all_of(m_data.begin(), m_data.end(),
                           [](uint8_t byte) {
                               char c = static_cast<char>(byte);
                               return isValidVinChar(c) || c == '0';
                           });
    }
};

// Definition of static Zero instance
inline const Vin Vin::Zero{};

/**
 * @brief Validate a VIN according to allowed characters (excluding I, O, Q)
 *
 * @param vin The Vin to validate
 * @return true if valid, false otherwise
 */
inline bool isValidVin(const Vin &vin) noexcept {
    return vin.isValid();
}

/**
 * @brief Stream output operator for Vin, EntityId, and GroupId
 *
 * @param os the operation stream
 * @param vin the Vin to output
 * @return std::ostream& the operation stream
 */
inline std::ostream &operator<<(std::ostream &os, const Vin &vin) {
    os << vin.toString();
    return os;
}

} // namespace doip
#endif /* VIN_H */
