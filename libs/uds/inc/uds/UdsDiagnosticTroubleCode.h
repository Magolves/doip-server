#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include <ostream>

namespace doip::uds {

/**
 * @brief Represents a Diagnostic Trouble Code (DTC) according to ISO 14229-1.
 *
 * A DTC consists of:
 * - 3-byte DTC code (24 bits)
 * - Status bits (1 byte): availability mask and actual status
 *
 * Status bit meanings (ISO 14229-1):
 * - Bit 0: testFailed
 * - Bit 1: testFailedThisOperationCycle
 * - Bit 2: pendingDTC
 * - Bit 3: confirmedDTC
 * - Bit 4: testNotCompletedSinceLastClear
 * - Bit 5: testFailedSinceLastClear
 * - Bit 6: testNotCompletedThisOperationCycle
 * - Bit 7: warningIndicatorRequested
 */
class DiagnosticTroubleCode {
public:
    // Status bit flags
    static constexpr uint8_t STATUS_TEST_FAILED = 0x01;
    static constexpr uint8_t STATUS_TEST_FAILED_THIS_CYCLE = 0x02;
    static constexpr uint8_t STATUS_PENDING_DTC = 0x04;
    static constexpr uint8_t STATUS_CONFIRMED_DTC = 0x08;
    static constexpr uint8_t STATUS_TEST_NOT_COMPLETED_SINCE_CLEAR = 0x10;
    static constexpr uint8_t STATUS_TEST_FAILED_SINCE_CLEAR = 0x20;
    static constexpr uint8_t STATUS_TEST_NOT_COMPLETED_THIS_CYCLE = 0x40;
    static constexpr uint8_t STATUS_WARNING_INDICATOR = 0x80;

    /**
     * @brief DTC severity levels
     */
    enum class Severity : uint8_t {
        Informational = 0x00,
        Warning = 0x01,
        Error = 0x02,
        Critical = 0x03
    };

    /**
     * @brief Default constructor - creates an invalid DTC with code 0x000000
     */
    DiagnosticTroubleCode() noexcept = default;

    /**
     * @brief Construct a DTC from a 3-byte code and status byte
     * @param code 24-bit DTC code (3 bytes)
     * @param statusBits Status byte with status flags
     */
    DiagnosticTroubleCode(uint32_t code, uint8_t statusBits) noexcept;

    /**
     * @brief Construct a DTC from individual bytes
     * @param highByte High byte of DTC code
     * @param middleByte Middle byte of DTC code
     * @param lowByte Low byte of DTC code
     * @param statusBits Status byte
     */
    DiagnosticTroubleCode(uint8_t highByte, uint8_t middleByte, uint8_t lowByte,
                          uint8_t statusBits) noexcept;

    /**
     * @brief Get the full 24-bit DTC code
     * @return DTC code (only lower 24 bits are valid)
     */
    [[nodiscard]] uint32_t getCode() const noexcept { return m_code & 0xFFFFFF; }

    /**
     * @brief Get high byte of DTC code
     */
    [[nodiscard]] uint8_t getHighByte() const noexcept {
        return static_cast<uint8_t>((m_code >> 16) & 0xFF);
    }

    /**
     * @brief Get middle byte of DTC code
     */
    [[nodiscard]] uint8_t getMiddleByte() const noexcept {
        return static_cast<uint8_t>((m_code >> 8) & 0xFF);
    }

    /**
     * @brief Get low byte of DTC code
     */
    [[nodiscard]] uint8_t getLowByte() const noexcept {
        return static_cast<uint8_t>(m_code & 0xFF);
    }

    /**
     * @brief Get the status byte
     */
    [[nodiscard]] uint8_t getStatusBits() const noexcept { return m_statusBits; }

    /**
     * @brief Set the status byte
     */
    void setStatusBits(uint8_t bits) noexcept { m_statusBits = bits; }

    /**
     * @brief Set individual status bit
     */
    void setStatusBit(uint8_t mask) noexcept { m_statusBits |= mask; }

    /**
     * @brief Clear individual status bit
     */
    void clearStatusBit(uint8_t mask) noexcept { m_statusBits &= ~mask; }

    /**
     * @brief Check if a specific status bit is set
     */
    [[nodiscard]] bool hasStatusBit(uint8_t mask) const noexcept {
        return (m_statusBits & mask) != 0;
    }

    /**
     * @brief Check if DTC is confirmed (both testFailed and confirmedDTC bits set)
     */
    [[nodiscard]] bool isConfirmed() const noexcept;

    /**
     * @brief Check if DTC is pending (pendingDTC bit set but not confirmedDTC)
     */
    [[nodiscard]] bool isPending() const noexcept;

    /**
     * @brief Check if DTC has an active failure (testFailed bit set)
     */
    [[nodiscard]] bool hasActiveFailure() const noexcept {
        return hasStatusBit(STATUS_TEST_FAILED);
    }

    /**
     * @brief Get severity level based on DTC code
     * @return Severity level
     */
    [[nodiscard]] Severity getSeverity() const noexcept;

    /**
     * @brief Serialize DTC to byte array (4 bytes: code + status)
     * @return Array of 4 bytes
     */
    [[nodiscard]] std::array<uint8_t, 4> serialize() const noexcept;

    /**
     * @brief Deserialize DTC from byte array
     * @param data Array of at least 4 bytes
     * @return true if deserialization was successful
     */
    bool deserialize(const std::array<uint8_t, 4>& data) noexcept;

    /**
     * @brief Deserialize DTC from vector of bytes
     * @param data Vector of bytes (must have at least 4 bytes)
     * @param offset Starting offset in the vector
     * @return true if deserialization was successful
     */
    bool deserialize(const std::vector<uint8_t>& data, size_t offset = 0) noexcept;

    /**
     * @brief Check if this DTC is valid (code is not 0x000000)
     */
    [[nodiscard]] bool isValid() const noexcept { return getCode() != 0x000000; }

    /**
     * @brief Get textual representation of status bits
     * @return String describing active status bits
     */
    [[nodiscard]] std::string getStatusDescription() const noexcept;

    // Comparison operators for sorting and searching
    [[nodiscard]] bool operator==(const DiagnosticTroubleCode& other) const noexcept {
        return m_code == other.m_code && m_statusBits == other.m_statusBits;
    }

    [[nodiscard]] bool operator!=(const DiagnosticTroubleCode& other) const noexcept {
        return !(*this == other);
    }

    [[nodiscard]] bool operator<(const DiagnosticTroubleCode& other) const noexcept {
        return m_code < other.m_code;
    }

    [[nodiscard]] bool operator<=(const DiagnosticTroubleCode& other) const noexcept {
        return m_code <= other.m_code;
    }

    [[nodiscard]] bool operator>(const DiagnosticTroubleCode& other) const noexcept {
        return m_code > other.m_code;
    }

    [[nodiscard]] bool operator>=(const DiagnosticTroubleCode& other) const noexcept {
        return m_code >= other.m_code;
    }

    /**
     * @brief Stream output operator
     */
    friend std::ostream& operator<<(std::ostream& os,
                                     const DiagnosticTroubleCode& dtc);

private:
    uint32_t m_code = 0x000000;      ///< 24-bit DTC code
    uint8_t m_statusBits = 0x00;     ///< Status bits
};

/**
 * @brief DTC Store - Collection of DTCs with filtering and search capabilities
 */
class DiagnosticTroubleCodeStore {
public:
    /**
     * @brief Add a DTC to the store
     * @param dtc The DTC to add
     * @return true if added, false if already exists
     */
    bool addDTC(const DiagnosticTroubleCode& dtc) noexcept;

    /**
     * @brief Remove a DTC from the store
     * @param code DTC code to remove
     * @return true if removed, false if not found
     */
    bool removeDTC(uint32_t code) noexcept;

    /**
     * @brief Clear all DTCs
     */
    void clearAll() noexcept { m_dtcs.clear(); }

    /**
     * @brief Get number of stored DTCs
     */
    [[nodiscard]] size_t count() const noexcept { return m_dtcs.size(); }

    /**
     * @brief Check if a DTC exists
     */
    [[nodiscard]] bool hasDTC(uint32_t code) const noexcept;

    /**
     * @brief Find a DTC by code
     * @param code DTC code to find
     * @return Pointer to DTC or nullptr if not found
     */
    [[nodiscard]] DiagnosticTroubleCode* findDTC(uint32_t code) noexcept;

    /**
     * @brief Find a DTC by code (const version)
     */
    [[nodiscard]] const DiagnosticTroubleCode* findDTC(uint32_t code) const noexcept;

    /**
     * @brief Get all confirmed DTCs
     */
    [[nodiscard]] std::vector<DiagnosticTroubleCode> getConfirmedDTCs() const noexcept;

    /**
     * @brief Get all pending DTCs
     */
    [[nodiscard]] std::vector<DiagnosticTroubleCode> getPendingDTCs() const noexcept;

    /**
     * @brief Get all DTCs with active failures
     */
    [[nodiscard]] std::vector<DiagnosticTroubleCode> getActiveDTCs() const noexcept;

    /**
     * @brief Get all DTCs
     */
    [[nodiscard]] const std::vector<DiagnosticTroubleCode>& getAllDTCs() const noexcept {
        return m_dtcs;
    }

    /**
     * @brief Serialize all DTCs to byte format
     * @return Vector of bytes (each DTC is 4 bytes)
     */
    [[nodiscard]] std::vector<uint8_t> serialize() const noexcept;

    /**
     * @brief Get DTC by index
     * @param index Index in the store
     * @return Pointer to DTC or nullptr if out of bounds
     */
    [[nodiscard]] DiagnosticTroubleCode* getDTCAt(size_t index) noexcept;

    /**
     * @brief Get DTC by index (const version)
     */
    [[nodiscard]] const DiagnosticTroubleCode* getDTCAt(size_t index) const noexcept;

private:
    std::vector<DiagnosticTroubleCode> m_dtcs;
};

} // namespace doip::uds
