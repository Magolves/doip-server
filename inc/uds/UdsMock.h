#ifndef UDSMOCK_H
#define UDSMOCK_H

#include <array>
#include <functional>
#include <memory>
#include <unordered_map>

#include "UdsServiceHandler.h"
#include "LambdaUdsHandler.h"
#include "UdsResponseCode.h"
#include "UdsServices.h"



using namespace doip;

namespace doip::uds {

constexpr uint8_t UDS_POSITIVE_RESPONSE_OFFSET = 0x40;

class UdsMock {
  public:
    UdsMock() = default;

    template <typename HandlerT>
    void registerService(UdsService serviceId) {
        m_handlers[static_cast<uint8_t>(serviceId)] = std::make_unique<HandlerT>();
    }

    // Register a lambda/function
    void registerService(UdsService serviceId, std::function<UdsResponse(const ByteArray &, const UniqueUdsModelPtr&)> fn) {
        m_handlers[static_cast<uint8_t>(serviceId)] = std::make_unique<LambdaUdsHandler>(std::move(fn));
    }

    // Backward-compatible overload: register lambda without model
    void registerService(UdsService serviceId, std::function<UdsResponse(const ByteArray &)> fn) {
        m_handlers[static_cast<uint8_t>(serviceId)] = std::make_unique<LambdaUdsHandler>(
            [f = std::move(fn)](const ByteArray &request, const UniqueUdsModelPtr&) {
                return f(request);
            }
        );
    }

    // Unregister
    void unregisterService(UdsService serviceId) {
        m_handlers.erase(static_cast<uint8_t>(serviceId));
    }

    // Convenience: clear all
    void clear() { m_handlers.clear(); }

    ByteArray handleDiagnosticRequest(const ByteArray &request) const;

    void registerDefaultServices();

  private:
    static ByteArray makeResponse(const ByteArray &request, UdsResponseCode responseCode = UdsResponseCode::OK, const ByteArray &extraData = {}) {
        if (responseCode != UdsResponseCode::OK) {
            ByteArray negativeResponse;
            negativeResponse.emplace_back(0x7F);                                // Negative response indicator
            negativeResponse.emplace_back(request.empty() ? 0x00 : request[0]); // Original service ID or 0
            negativeResponse.emplace_back(static_cast<uint8_t>(responseCode));  // NRC
            return negativeResponse;
        }

        ByteArray positiveResponse;
        positiveResponse.emplace_back(static_cast<uint8_t>((request.empty() ? 0x00 : request[0]) + UDS_POSITIVE_RESPONSE_OFFSET)); // Positive response SID
        positiveResponse.insert(positiveResponse.end(), extraData.begin(), extraData.end());
        return positiveResponse;
    }

    std::unordered_map<uint8_t, UniqueUdsServiceHandlerPtr> m_handlers;

    UniqueUdsModelPtr m_model{nullptr};
};

} // namespace doip::uds

#endif /* UDSMOCK_H */
