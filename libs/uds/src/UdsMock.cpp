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
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/ostr.h> // For fmt::streamed() support

namespace doip::uds {

/**
 * @brief Register default services that respond with "Service Not Supported"
 */
void UdsMock::registerDefaultServices() {
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
        m_logger->error("Unknown service ID 0x{:02X}", sid);
        return UdsServiceHandler::makeNegativeResponse(UdsResponseCode::ServiceNotSupported, request);
    }

    if (request.size() < desc->minReqLength || request.size() > desc->maxReqLength) {
        m_logger->error("Request length {} out of bounds for service 0x{:02X} (expected {}-{}, req={})",
                         request.size(), sid, desc->minReqLength, desc->maxReqLength, fmt::streamed(request));
        return UdsServiceHandler::makeNegativeResponse(UdsResponseCode::IncorrectMessageLengthOrInvalidFormat, request);
    }

    ByteArray response = {};
    auto it = m_handlers.find(sid);
    if (it != m_handlers.end() && it->second) {
        response = it->second->handle(request, m_model);
    } else {
        m_logger->error("Unsupported service ID 0x{:02X}", sid);
        return UdsServiceHandler::makeNegativeResponse(UdsResponseCode::ServiceNotSupported, request);
    }

    if (isNegativeResponse(response)) {
        return response; // already a negative response
    }

    auto rspSize = response.size(); // +1 for the SID
    if (rspSize < desc->minRspLength || rspSize > desc->maxRspLength) {
        m_logger->error("Response length {} out of bounds for service 0x{:02X} (expected {}-{}, rsp={})",
                            response.size(), sid, desc->minRspLength, desc->maxRspLength, fmt::streamed(response));
        return UdsServiceHandler::makeNegativeResponse(UdsResponseCode::GeneralProgrammingFailure, request);
    }

    return response;
}

} // namespace doip::uds
