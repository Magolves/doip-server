#pragma once

#include <cstdint>

namespace doip::uds {

/**
 * ReadDTCInformation (SID 0x19) subfunctions per ISO 14229-1, section 12.3.
 * Availability may vary by ECU/configuration; values are standard IDs.
 *
 */
enum class ReadDTCInformationSubFunction : uint8_t {
    ReportNumberOfDTCByStatusMask                 = 0x01, // LEV_RNOTCBSM
    ReportDTCByStatusMask                         = 0x02, // LEV_RTCBSM
    ReportDTCSnapshotRecordByDTCNumber            = 0x03,
    ReportDTCSnapshotRecordByRecordNumber         = 0x04,
    ReportDTCSnapshotIdentification               = 0x05,
    ReportDTCExtendedDataRecordByDTCNumber        = 0x06,
    ReportNumberOfDTCBySeverityMask               = 0x07,
    ReportDTCBySeverityMask                       = 0x08,
    ReportSeverityInformationOfDTC                = 0x09,
    ReportSupportedDTC                            = 0x0A,
    // 0x0B reserved/unassigned in many editions
    ReportMirrorMemoryDTCByStatusMask             = 0x0C,
    ReportNumberOfMirrorMemoryDTCByStatusMask     = 0x0D,
    ReportMirrorMemoryDTCExtendedDataRecordByDTCNumber = 0x0E,
    ReportNumberOfEmissionsRelatedDTCByStatusMask = 0x0F,
    ReportEmissionsRelatedDTCByStatusMask         = 0x10,
    ReportDTCFaultDetectionCounter                = 0x11,
    ReportDTCWithPermanentStatus                  = 0x12,
};

constexpr bool isValidReadDTCInformationSubFunction(uint8_t subFunction) {
    switch (static_cast<ReadDTCInformationSubFunction>(subFunction)) {
        case ReadDTCInformationSubFunction::ReportNumberOfDTCByStatusMask:
        case ReadDTCInformationSubFunction::ReportDTCByStatusMask:
        case ReadDTCInformationSubFunction::ReportDTCSnapshotRecordByDTCNumber:
        case ReadDTCInformationSubFunction::ReportDTCSnapshotRecordByRecordNumber:
        case ReadDTCInformationSubFunction::ReportDTCSnapshotIdentification:
        case ReadDTCInformationSubFunction::ReportDTCExtendedDataRecordByDTCNumber:
        case ReadDTCInformationSubFunction::ReportNumberOfDTCBySeverityMask:
        case ReadDTCInformationSubFunction::ReportDTCBySeverityMask:
        case ReadDTCInformationSubFunction::ReportSeverityInformationOfDTC:
        case ReadDTCInformationSubFunction::ReportSupportedDTC:
        case ReadDTCInformationSubFunction::ReportMirrorMemoryDTCByStatusMask:
        case ReadDTCInformationSubFunction::ReportNumberOfMirrorMemoryDTCByStatusMask:
        case ReadDTCInformationSubFunction::ReportMirrorMemoryDTCExtendedDataRecordByDTCNumber:
        case ReadDTCInformationSubFunction::ReportNumberOfEmissionsRelatedDTCByStatusMask:
        case ReadDTCInformationSubFunction::ReportEmissionsRelatedDTCByStatusMask:
        case ReadDTCInformationSubFunction::ReportDTCFaultDetectionCounter:
        case ReadDTCInformationSubFunction::ReportDTCWithPermanentStatus:
            return true;
        default:
            return false;
    }
}

constexpr bool isSupportedSubFunction(ReadDTCInformationSubFunction subFunction) {
    switch (subFunction) {
        case ReadDTCInformationSubFunction::ReportNumberOfDTCByStatusMask:
        case ReadDTCInformationSubFunction::ReportDTCByStatusMask:
        // case ReadDTCInformationSubFunction::ReportDTCSnapshotRecordByDTCNumber:
        // case ReadDTCInformationSubFunction::ReportDTCSnapshotRecordByRecordNumber:
        // case ReadDTCInformationSubFunction::ReportDTCSnapshotIdentification:
        // case ReadDTCInformationSubFunction::ReportDTCExtendedDataRecordByDTCNumber:
        case ReadDTCInformationSubFunction::ReportNumberOfDTCBySeverityMask:
        case ReadDTCInformationSubFunction::ReportDTCBySeverityMask:
        // case ReadDTCInformationSubFunction::ReportSeverityInformationOfDTC:
        // case ReadDTCInformationSubFunction::ReportSupportedDTC:
        // case ReadDTCInformationSubFunction::ReportMirrorMemoryDTCByStatusMask:
        // case ReadDTCInformationSubFunction::ReportNumberOfMirrorMemoryDTCByStatusMask:
        // case ReadDTCInformationSubFunction::ReportMirrorMemoryDTCExtendedDataRecordByDTCNumber:
        // case ReadDTCInformationSubFunction::ReportNumberOfEmissionsRelatedDTCByStatusMask:
        // case ReadDTCInformationSubFunction::ReportEmissionsRelatedDTCByStatusMask:
        // case ReadDTCInformationSubFunction::ReportDTCFaultDetectionCounter:
        // case ReadDTCInformationSubFunction::ReportDTCWithPermanentStatus:
            return true;
        default:
            return false;
    }
}

constexpr bool isCountSubFunction(ReadDTCInformationSubFunction subFunction) {
    switch (subFunction) {
        case ReadDTCInformationSubFunction::ReportNumberOfDTCByStatusMask:
        case ReadDTCInformationSubFunction::ReportNumberOfDTCBySeverityMask:
        case ReadDTCInformationSubFunction::ReportNumberOfMirrorMemoryDTCByStatusMask:
        case ReadDTCInformationSubFunction::ReportNumberOfEmissionsRelatedDTCByStatusMask:
            return true;
        default:
            return false;
    }
}

constexpr bool isQuerySubFunction(ReadDTCInformationSubFunction subFunction) {
    switch (subFunction) {
        case ReadDTCInformationSubFunction::ReportDTCSnapshotRecordByDTCNumber:
        case ReadDTCInformationSubFunction::ReportDTCSnapshotRecordByRecordNumber:
        case ReadDTCInformationSubFunction::ReportDTCSnapshotIdentification:
        case ReadDTCInformationSubFunction::ReportDTCExtendedDataRecordByDTCNumber:
        case ReadDTCInformationSubFunction::ReportSeverityInformationOfDTC:
        case ReadDTCInformationSubFunction::ReportMirrorMemoryDTCExtendedDataRecordByDTCNumber:
        case ReadDTCInformationSubFunction::ReportDTCFaultDetectionCounter:
            return true;
        default:
            return false;
    }
}

constexpr bool returnsDTCs(ReadDTCInformationSubFunction subFunction) {
    switch (subFunction) {
        case ReadDTCInformationSubFunction::ReportDTCByStatusMask:
        case ReadDTCInformationSubFunction::ReportDTCBySeverityMask:
        case ReadDTCInformationSubFunction::ReportSupportedDTC:
        case ReadDTCInformationSubFunction::ReportMirrorMemoryDTCByStatusMask:
        case ReadDTCInformationSubFunction::ReportEmissionsRelatedDTCByStatusMask:
        case ReadDTCInformationSubFunction::ReportDTCWithPermanentStatus:
            return true;
        default:
            return false;
    }
}

} // namespace doip::uds
