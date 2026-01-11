#include <doctest/doctest.h>
#include <vector>
#include <unordered_map>

#include "uds/services/UdsReadDTCInformationSubFunction.h"

namespace doip::uds {

TEST_SUITE("DiagnosticTroubleCode-UdsReadDTCInformationSubFunction") {
    struct ReadDTCInformationSubFunctionTest {
        bool expectedIsSupported;
        bool expectedIsCount;
        bool expectedIsQuery;
        bool expectedReturnsDTCs;
    };

    struct UdsReadDTCInformationSubFunctionFixture {
        std::vector<ReadDTCInformationSubFunction> subFunctions = {
            ReadDTCInformationSubFunction::ReportNumberOfDTCByStatusMask,
            ReadDTCInformationSubFunction::ReportDTCByStatusMask,
            ReadDTCInformationSubFunction::ReportDTCSnapshotIdentification,
            ReadDTCInformationSubFunction::ReportDTCSnapshotRecordByDTCNumber,
            ReadDTCInformationSubFunction::ReportDTCExtendedDataRecordByDTCNumber,
            ReadDTCInformationSubFunction::ReportNumberOfDTCBySeverityMask,
            ReadDTCInformationSubFunction::ReportDTCBySeverityMask,
            ReadDTCInformationSubFunction::ReportSeverityInformationOfDTC,
            ReadDTCInformationSubFunction::ReportSupportedDTC,
            ReadDTCInformationSubFunction::ReportMirrorMemoryDTCByStatusMask,
            ReadDTCInformationSubFunction::ReportNumberOfMirrorMemoryDTCByStatusMask,
            ReadDTCInformationSubFunction::ReportMirrorMemoryDTCExtendedDataRecordByDTCNumber,
            ReadDTCInformationSubFunction::ReportNumberOfEmissionsRelatedDTCByStatusMask,
            ReadDTCInformationSubFunction::ReportEmissionsRelatedDTCByStatusMask,
            ReadDTCInformationSubFunction::ReportDTCFaultDetectionCounter,
            ReadDTCInformationSubFunction::ReportDTCWithPermanentStatus
        };

        std::unordered_map<ReadDTCInformationSubFunction, ReadDTCInformationSubFunctionTest> testCases = {
            { ReadDTCInformationSubFunction::ReportNumberOfDTCByStatusMask, { true, true, false, false } },
            { ReadDTCInformationSubFunction::ReportDTCByStatusMask, { true, false, false, true } },
            { ReadDTCInformationSubFunction::ReportDTCSnapshotIdentification, { false, false, true, false } },
            { ReadDTCInformationSubFunction::ReportDTCSnapshotRecordByDTCNumber, { false, false, true, false } },
            { ReadDTCInformationSubFunction::ReportDTCExtendedDataRecordByDTCNumber, { false, false, true, false } },
            { ReadDTCInformationSubFunction::ReportNumberOfDTCBySeverityMask, { true, true, false, false } },
            { ReadDTCInformationSubFunction::ReportDTCBySeverityMask, { true, false, false, true } },
            { ReadDTCInformationSubFunction::ReportSeverityInformationOfDTC, { false, false, true, false } },
            { ReadDTCInformationSubFunction::ReportSupportedDTC, { false, false, false, true } },
            { ReadDTCInformationSubFunction::ReportMirrorMemoryDTCByStatusMask, { false, false, false, true } },
            { ReadDTCInformationSubFunction::ReportNumberOfMirrorMemoryDTCByStatusMask, { false, true, false, false } },
            { ReadDTCInformationSubFunction::ReportMirrorMemoryDTCExtendedDataRecordByDTCNumber, { false, false, true, false } },
            { ReadDTCInformationSubFunction::ReportNumberOfEmissionsRelatedDTCByStatusMask, { false, true, false, false } },
            { ReadDTCInformationSubFunction::ReportEmissionsRelatedDTCByStatusMask, { false, false, false, true } },
            { ReadDTCInformationSubFunction::ReportDTCFaultDetectionCounter, { false, false, true, false } },
            { ReadDTCInformationSubFunction::ReportDTCWithPermanentStatus, { false, false, false, true } }
        };


        UdsReadDTCInformationSubFunctionFixture() {
            // Setup code here if needed
        }

        ~UdsReadDTCInformationSubFunctionFixture() {
            // Cleanup code here if needed
        }
    };


    TEST_CASE_FIXTURE(UdsReadDTCInformationSubFunctionFixture, "ReadDTCInformationSubFunction properties") {
        for (const auto& subFunction : subFunctions) {
            const auto& testCase = testCases[subFunction];

            CHECK(isSupportedSubFunction(subFunction) == testCase.expectedIsSupported);
            CHECK(isCountSubFunction(subFunction) == testCase.expectedIsCount);
            CHECK(isQuerySubFunction(subFunction) == testCase.expectedIsQuery);
            CHECK(returnsDTCs(subFunction) == testCase.expectedReturnsDTCs);
        }

    }
    
}

} // namespace doip::uds