#include "uds/UdsMock.h"


#include "uds/services/UdsDiagnosticSessionControl.h"
#include "uds/services/UdsECUReset.h"
#include "uds/services/UdsReadDataByIdentifier.h"
#include "uds/services/UdsRequestDownload.h"
#include "uds/services/UdsRequestTransferExit.h"
#include "uds/services/UdsSecurityAccess.h"
#include "uds/services/UdsTesterPresent.h"
#include "uds/services/UdsTransferData.h"
#include "uds/services/UdsWriteDataByIdentifier.h"

namespace doip::uds {

     /**
     * @brief Register default services that respond with "Service Not Supported"
     */
    void UdsMock::registerDefaultServices() {
        const std::array<UdsService, 19> services = {
            UdsService::DiagnosticSessionControl,
            UdsService::ECUReset,
            UdsService::SecurityAccess,
            UdsService::CommunicationControl,
            UdsService::TesterPresent,
            UdsService::AccessTimingParameters,
            UdsService::SecuredDataTransmission,
            UdsService::ControlDTCSetting,
            UdsService::ResponseOnEvent,
            UdsService::LinkControl,
            UdsService::ReadDataByIdentifier,
            UdsService::ReadMemoryByAddress,
            UdsService::ReadScalingDataByIdentifier,
            UdsService::ReadDataByPeriodicIdentifier,
            UdsService::DynamicallyDefineDataIdentifier,
            UdsService::WriteDataByIdentifier,
            UdsService::WriteMemoryByAddress,
            UdsService::ClearDiagnosticInformation,
            UdsService::ReadDTCInformation,
        };

        for (auto s : services) {
            registerService(s, [](const ByteArray &req, const UniqueUdsModelPtr&) -> ByteArray {
                (void)req;
                return UdsServiceHandler::makeNegativeResponse(UdsResponseCode::ServiceNotSupported, req);
            });
        }

        registerService<DiagnosticSessionControlHandler>(UdsService::DiagnosticSessionControl);
        registerService<ECUResetHandler>(UdsService::ECUReset);
        registerService<ReadDataByIdentifierHandler>(UdsService::ReadDataByIdentifier);
        registerService<RequestDownloadHandler>(UdsService::RequestDownload);
        registerService<RequestTransferExitHandler>(UdsService::RequestTransferExit);
        registerService<SecurityAccessHandler>(UdsService::SecurityAccess);
        registerService<TesterPresentHandler>(UdsService::TesterPresent);
        registerService<TransferDataHandler>(UdsService::TransferData);
        registerService<WriteDataByIdentifierHandler>(UdsService::WriteDataByIdentifier);
    }

ByteArray UdsMock::handleDiagnosticRequest(const ByteArray &request) const {
    if (request.empty())
        return {};
    uint8_t sid = request[0];

    const UdsServiceDescriptor *desc = findServiceDescriptor(sid);
    if (!desc) {
        return UdsServiceHandler::makeNegativeResponse(UdsResponseCode::ServiceNotSupported, request);
    }

    if (request.size() < desc->minReqLength || request.size() > desc->maxReqLength) {
        std::cerr << "UdsMock: Request length " << request.size()
                  << " out of bounds for service 0x" << std::hex << +sid << std::dec
                  << " (expected " << desc->minReqLength << "-" << desc->maxReqLength << ")\n";
        return UdsServiceHandler::makeNegativeResponse(UdsResponseCode::IncorrectMessageLengthOrInvalidFormat, request);
    }

    ByteArray response = {};
    auto it = m_handlers.find(sid);
    if (it != m_handlers.end() && it->second) {
        response = it->second->handle(request, m_model);
    } else {
        return UdsServiceHandler::makeNegativeResponse(UdsResponseCode::ServiceNotSupported, request);
    }

    if (isNegativeResponse(response)) {
        return response; // already a negative response
    }

    auto rspSize = response.size(); // +1 for the SID
    if (rspSize < desc->minRspLength || rspSize > desc->maxRspLength) {
        std::cerr << "UdsMock: Response length " << response.size()
                  << " out of bounds for service 0x" << std::hex << +sid << std::dec
                  << " (expected " << desc->minRspLength << "-" << desc->maxRspLength << ")\n";
        return UdsServiceHandler::makeNegativeResponse(UdsResponseCode::GeneralProgrammingFailure, request);
    }

    return response;
}

} // namespace doip::uds
