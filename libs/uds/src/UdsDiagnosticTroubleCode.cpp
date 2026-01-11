#include "uds/UdsDiagnosticTroubleCode.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace doip::uds {

DiagnosticTroubleCode::DiagnosticTroubleCode(uint32_t code, uint8_t statusBits) noexcept
    : m_code(code & 0xFFFFFF), m_statusBits(statusBits) {}

DiagnosticTroubleCode::DiagnosticTroubleCode(uint8_t highByte, uint8_t middleByte,
                                             uint8_t lowByte, uint8_t statusBits) noexcept
    : m_code((static_cast<uint32_t>(highByte) << 16) |
             (static_cast<uint32_t>(middleByte) << 8) | static_cast<uint32_t>(lowByte)),
      m_statusBits(statusBits) {}

bool DiagnosticTroubleCode::isConfirmed() const noexcept {
    return hasStatusBit(STATUS_TEST_FAILED) && hasStatusBit(STATUS_CONFIRMED_DTC);
}

bool DiagnosticTroubleCode::isPending() const noexcept {
    return hasStatusBit(STATUS_PENDING_DTC) && !hasStatusBit(STATUS_CONFIRMED_DTC);
}

DiagnosticTroubleCode::Severity DiagnosticTroubleCode::getSeverity() const noexcept {
    // Severity is typically encoded in the high byte of the DTC code
    // Bits 6-7 of the high byte: 00=informational, 01=warning, 10=error, 11=critical
    uint8_t high = getHighByte();
    uint8_t severityBits = (high >> 6) & 0x03;

    switch (severityBits) {
    case 0x01:
        return Severity::Warning;
    case 0x02:
        return Severity::Error;
    case 0x03:
        return Severity::Critical;
    default:
        return Severity::Informational;
    }
}

std::array<uint8_t, 4> DiagnosticTroubleCode::serialize() const noexcept {
    return {getHighByte(), getMiddleByte(), getLowByte(), m_statusBits};
}

bool DiagnosticTroubleCode::deserialize(const std::array<uint8_t, 4> &data) noexcept {
    m_code = (static_cast<uint32_t>(data[0]) << 16) | (static_cast<uint32_t>(data[1]) << 8) |
             static_cast<uint32_t>(data[2]);
    m_statusBits = data[3];
    return true;
}

bool DiagnosticTroubleCode::deserialize(const std::vector<uint8_t> &data,
                                        size_t offset) noexcept {
    if (data.size() < offset + 4) {
        return false;
    }

    std::array<uint8_t, 4> arr = {data[offset], data[offset + 1], data[offset + 2],
                                  data[offset + 3]};
    return deserialize(arr);
}

std::string DiagnosticTroubleCode::getStatusDescription() const noexcept {
    std::ostringstream oss;
    std::vector<std::string> tags;

    if (hasStatusBit(STATUS_TEST_FAILED)) {
        tags.emplace_back("testFailed");
    }

    if (hasStatusBit(STATUS_TEST_FAILED_THIS_CYCLE)) {
        tags.emplace_back("testFailedThisOperationCycle");
    }

    if (hasStatusBit(STATUS_PENDING_DTC)) {
        tags.emplace_back("pendingDTC");
    }

    if (hasStatusBit(STATUS_CONFIRMED_DTC)) {
        tags.emplace_back("confirmedDTC");
    }

    if (hasStatusBit(STATUS_TEST_NOT_COMPLETED_SINCE_CLEAR)) {
        tags.emplace_back("testNotCompletedSinceClear");
    }

    if (hasStatusBit(STATUS_TEST_FAILED_SINCE_CLEAR)) {
        tags.emplace_back("testFailedSinceClear");
    }

    if (hasStatusBit(STATUS_TEST_NOT_COMPLETED_THIS_CYCLE)) {
        tags.emplace_back("testNotCompletedThisOperationCycle");
    }

    if (hasStatusBit(STATUS_WARNING_INDICATOR)) {
        tags.emplace_back("warningIndicatorRequested");
    }

    bool first = true;
    for (const auto &tag : tags) {
        if (!first)
            oss << ", ";
        oss << tag;
        first = false;
    }

    return oss.str();
}

std::ostream &operator<<(std::ostream &os, const DiagnosticTroubleCode &dtc) {
    os << "DTC(0x" << std::hex << std::setfill('0') << std::setw(6) << dtc.getCode()
       << ", status=0x" << std::setw(2) << static_cast<int>(dtc.getStatusBits()) << std::dec
       << ")";
    return os;
}

// DiagnosticTroubleCodeStore implementation

bool DiagnosticTroubleCodeStore::addDTC(const DiagnosticTroubleCode &dtc) noexcept {
    if (hasDTC(dtc.getCode())) {
        return false;
    }
    m_dtcs.push_back(dtc);
    std::sort(m_dtcs.begin(), m_dtcs.end());
    return true;
}

bool DiagnosticTroubleCodeStore::removeDTC(uint32_t code) noexcept {
    auto it = std::find_if(m_dtcs.begin(), m_dtcs.end(),
                           [code](const DiagnosticTroubleCode &dtc) {
                               return dtc.getCode() == code;
                           });

    if (it != m_dtcs.end()) {
        m_dtcs.erase(it);
        return true;
    }
    return false;
}

size_t DiagnosticTroubleCodeStore::countByCodeBits(uint8_t codeMask) const noexcept {
    return static_cast<size_t>(std::count_if(m_dtcs.begin(), m_dtcs.end(),
                                             [codeMask](const DiagnosticTroubleCode &dtc) {
                                                 return (dtc.getCode() & codeMask) != 0;
                                             }));
}

size_t DiagnosticTroubleCodeStore::countByStatusBits(uint8_t statusMask) const noexcept {
    return static_cast<size_t>(std::count_if(m_dtcs.begin(), m_dtcs.end(),
                                             [statusMask](const DiagnosticTroubleCode &dtc) {
                                                 return (dtc.getStatusBits() & statusMask) != 0;
                                             }));
}

bool DiagnosticTroubleCodeStore::hasDTC(uint32_t code) const noexcept {
    return std::any_of(m_dtcs.begin(), m_dtcs.end(), [code](const DiagnosticTroubleCode &dtc) {
        return dtc.getCode() == code;
    });
}

std::optional<DiagnosticTroubleCode> DiagnosticTroubleCodeStore::findDTC(uint32_t code) const noexcept {
    auto it =
        std::find_if(m_dtcs.begin(), m_dtcs.end(), [code](const DiagnosticTroubleCode &dtc) {
            return dtc.getCode() == code;
        });

    return it != m_dtcs.end() ? std::optional<DiagnosticTroubleCode>(*it) : std::nullopt;
}

std::vector<DiagnosticTroubleCode> DiagnosticTroubleCodeStore::findDTCByStatus(uint8_t status) const noexcept {
    std::vector<DiagnosticTroubleCode> results;
    std::copy_if(m_dtcs.begin(), m_dtcs.end(), std::back_inserter(results), [status](const DiagnosticTroubleCode &dtc) {
        return dtc.getStatusBits() == status;
    });
    return results;
}


std::vector<DiagnosticTroubleCode> DiagnosticTroubleCodeStore::getConfirmedDTCs()
    const noexcept {
    std::vector<DiagnosticTroubleCode> confirmed;
    std::copy_if(m_dtcs.begin(), m_dtcs.end(), std::back_inserter(confirmed), [](const DiagnosticTroubleCode &dtc) { return dtc.isConfirmed(); });
    return confirmed;
}

std::vector<DiagnosticTroubleCode> DiagnosticTroubleCodeStore::getPendingDTCs()
    const noexcept {
    std::vector<DiagnosticTroubleCode> pending;
    std::copy_if(m_dtcs.begin(), m_dtcs.end(), std::back_inserter(pending), [](const DiagnosticTroubleCode &dtc) { return dtc.isPending(); });
    return pending;
}

std::vector<DiagnosticTroubleCode> DiagnosticTroubleCodeStore::getActiveDTCs() const noexcept {
    std::vector<DiagnosticTroubleCode> active;
    std::copy_if(m_dtcs.begin(), m_dtcs.end(), std::back_inserter(active), [](const DiagnosticTroubleCode &dtc) { return dtc.hasActiveFailure(); });
    return active;
}

std::vector<uint8_t> DiagnosticTroubleCodeStore::serialize() const noexcept {
    std::vector<uint8_t> result;
    result.reserve(m_dtcs.size() * 4);

    for (const auto &dtc : m_dtcs) {
        auto serialized = dtc.serialize();
        result.insert(result.end(), serialized.begin(), serialized.end());
    }

    return result;
}

DiagnosticTroubleCode *DiagnosticTroubleCodeStore::getDTCAt(size_t index) noexcept {
    if (index >= m_dtcs.size()) {
        return nullptr;
    }
    return &m_dtcs[index];
}

const DiagnosticTroubleCode *DiagnosticTroubleCodeStore::getDTCAt(size_t index) const noexcept {
    if (index >= m_dtcs.size()) {
        return nullptr;
    }
    return &m_dtcs[index];
}

} // namespace doip::uds
