#pragma once

#include "../UdsServiceHandler.h"
#include "../UdsServices.h"

namespace doip::uds {


class DiagnosticSessionControlHandler : public UdsServiceHandler {
public:
    ~DiagnosticSessionControlHandler() override = default;

    UdsResponse handle(const ByteArray& request, const UniqueUdsModelPtr& model) override {
        uint8_t sessionType = request[1];
        DiagnosticSessionControlType session;

        switch (sessionType) {
            case 0x01:
                session = DiagnosticSessionControlType::DefaultSession;
                break;
            case 0x02:
                session = DiagnosticSessionControlType::ProgrammingSession;
                break;
            case 0x03:
                session = DiagnosticSessionControlType::ExtendedDiagnosticSession;
                break;
            case 0x04:
                session = DiagnosticSessionControlType::SafetySystemDiagnosticSession;
                break;
            default:
                return makeNegativeResponse(UdsResponseCode::SubFunctionNotSupported, request);
        }

        UdsResponseCode result = model->setCurrentSession(session);
        if (result != UdsResponseCode::PositiveResponse) {
            return makeNegativeResponse(result, request);
        }

        ByteArray responseData;
        responseData.push_back(static_cast<uint8_t>(request[0] + UDS_POSITIVE_RESPONSE_OFFSET));
        responseData.push_back(sessionType);

        return makeResponse(request, responseData);
    }
protected:
    using UdsServiceHandler::makeResponse;
    using UdsServiceHandler::makeNegativeResponse;
};

} // namespace doip::uds
