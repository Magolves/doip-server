#pragma once

#include "../UdsServiceHandler.h"
#include "UdsReadDTCInformationSubFunction.h"

namespace doip::uds {

/**
 * @brief DTC format identifiers as per ISO 14229-1:2020, Table D14
 *
 */
enum class DtcFormatIdentifier : uint8_t {
    SAE_J2012_DA_DTCF00 = 0x00,
    ISO_14229_1_DTCF = 0x01,
    J1939_73_DTCF = 0x02,
    ISO_11992_4_DTCF = 0x03,
    J2012_DA_DTCF04 = 0x04,
};

/**
 * @brief Format identifier for DTCs as per ISO 14229-1:2020.
 *
 */
constexpr uint8_t DTC_FORMAT_IDENTIFIER = static_cast<uint8_t>(DtcFormatIdentifier::SAE_J2012_DA_DTCF00);

class ReadDTCInformationHandler : public UdsServiceHandler {
  public:
    ~ReadDTCInformationHandler() override = default;
    ByteArray handle(const ByteArray &request, const UniqueUdsModelPtr &model) override {
        uint8_t subFunction = request[1];

        if (!isSupportedSubFunction(static_cast<ReadDTCInformationSubFunction>(subFunction))) {
            return makeNegativeResponse(UdsResponseCode::SubFunctionNotSupported, request);
        }

        if (model->getCurrentSession() == DiagnosticSessionControlType::DefaultSession) {
            return makeNegativeResponse(UdsResponseCode::ServiceNotSupportedInActiveSession, request);
        }

        ByteArray responseData;
        responseData.writeU8(sidResponseCode(request[0]));
        responseData.writeU8(subFunction);

        auto &dtcStore = model.get()->getDTCStore();

        UdsResponseCode result;
        auto it = subFunctionHandlers.find(static_cast<ReadDTCInformationSubFunction>(subFunction));
        if (it != subFunctionHandlers.end()) {
            result = it->second(request, dtcStore, responseData);

            if (result != UdsResponseCode::PositiveResponse) {
                return makeNegativeResponse(result, request);
            }
            return responseData;
        } else {
            return makeNegativeResponse(UdsResponseCode::SubFunctionNotSupported, request);
        }

        return responseData;
    }

  protected:
    UdsResponseCode handleReportNumberOfDTCByStatusMask(const ByteArray &request, const DiagnosticTroubleCodeStore &store, ByteArray &responseData);
    UdsResponseCode handleReportDTCByStatusMask(const ByteArray &request, const DiagnosticTroubleCodeStore &store, ByteArray &responseData);

    const std::unordered_map<ReadDTCInformationSubFunction, std::function<UdsResponseCode(const ByteArray &request, const DiagnosticTroubleCodeStore &store, ByteArray &responseData)>> subFunctionHandlers = {
        {ReadDTCInformationSubFunction::ReportNumberOfDTCByStatusMask, [this](const ByteArray &request, const DiagnosticTroubleCodeStore &store, ByteArray &responseData) {
             return handleReportNumberOfDTCByStatusMask(request, store, responseData);
         }},
        {ReadDTCInformationSubFunction::ReportDTCByStatusMask, [this](const ByteArray &request, const DiagnosticTroubleCodeStore &store, ByteArray &responseData) {
             return handleReportDTCByStatusMask(request, store, responseData);
         }},
        // Add other sub-function handlers here
    };

    using UdsServiceHandler::makeNegativeResponse;
    using UdsServiceHandler::makeResponse;
};

} // namespace doip::uds
