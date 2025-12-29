#include "uds/UdsMock.h"


#include "uds/services/UdsDiagnosticSessionControl.h"
#include "uds/services/UdsECUReset.h"
#include "uds/services/UdsRequestDownload.h"
#include "uds/services/UdsRequestTransferExit.h"
#include "uds/services/UdsTransferData.h"
#include "uds/services/UdsWriteDataByIdentifier.h"
#include "uds/services/UdsTesterPresent.h"

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
            registerService(s, [](const ByteArray &req, const UniqueUdsModelPtr&) -> UdsResponse {
                (void)req;
                return {UdsResponseCode::ServiceNotSupported, {}};
            });
        }

        registerService<DiagnosticSessionControlHandler>(UdsService::DiagnosticSessionControl);
        registerService<ECUResetHandler>(UdsService::ECUReset);
        registerService<RequestTransferExitHandler>(UdsService::RequestTransferExit);
        registerService<RequestDownloadHandler>(UdsService::RequestDownload);
        registerService<TransferDataHandler>(UdsService::TransferData);
        registerService<WriteDataByIdentifierHandler>(UdsService::WriteDataByIdentifier);
        registerService<TesterPresentHandler>(UdsService::TesterPresent);
    }

ByteArray UdsMock::handleDiagnosticRequest(const ByteArray &request) const {
    if (request.empty())
        return {};
    uint8_t sid = request[0];
    UdsService service = static_cast<UdsService>(sid);

    const UdsServiceDescriptor *desc = findServiceDescriptor(service);
    if (!desc) {
        return makeResponse(request, UdsResponseCode::ServiceNotSupported, {});
    }

    if (request.size() < desc->minReqLength || request.size() > desc->maxReqLength) {
        std::cerr << "UdsMock: Request length " << request.size()
                  << " out of bounds for service 0x" << std::hex << static_cast<int>(service) << std::dec
                  << " (expected " << desc->minReqLength << "-" << desc->maxReqLength << ")\n";
        return makeResponse(request, UdsResponseCode::IncorrectMessageLengthOrInvalidFormat);
    }

    UdsResponse resp = {UdsResponseCode::ServiceNotSupported, {}};
    auto it = m_handlers.find(sid);
    if (it != m_handlers.end() && it->second) {
        resp = it->second->handle(request, m_model);
    } else {
        return makeResponse(request, UdsResponseCode::ServiceNotSupported);
    }

    auto rspSize = resp.second.size() + 1; // +1 for the SID
    if (rspSize < desc->minRspLength || rspSize > desc->maxRspLength) {
        std::cerr << "UdsMock: Response length " << resp.second.size()
                  << " out of bounds for service 0x" << std::hex << static_cast<int>(service) << std::dec
                  << " (expected " << desc->minRspLength << "-" << desc->maxRspLength << ")\n";
        return makeResponse(request, UdsResponseCode::GeneralProgrammingFailure, {});
    }

    return makeResponse(request, resp.first, resp.second);
}

} // namespace doip::uds
