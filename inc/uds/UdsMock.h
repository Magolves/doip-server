#ifndef UDSMOCK_H
#define UDSMOCK_H


#include "UdsServiceHandler.h"
#include "LambdaUdsHandler.h"
#include "UdsServices.h"
#include "Logger.h"

#include <functional>
#include <memory>
#include <unordered_map>



using namespace doip;

namespace doip::uds {

constexpr uint8_t UDS_POSITIVE_RESPONSE_OFFSET = 0x40;

class UdsMock {
  public:
    explicit UdsMock(UniqueUdsModelPtr model) : m_model(std::move(model)) {}

    template <typename HandlerT>
    void registerService(UdsService serviceId) {
        m_handlers[static_cast<uint8_t>(serviceId)] = std::make_unique<HandlerT>();
    }

    // Register a lambda/function
    void registerService(UdsService serviceId, std::function<ByteArray(const ByteArray &, const UniqueUdsModelPtr&)> fn) {
        m_handlers[static_cast<uint8_t>(serviceId)] = std::make_unique<LambdaUdsHandler>(std::move(fn));
    }

    // Backward-compatible overload: register lambda without model
    void registerService(UdsService serviceId, std::function<ByteArray(const ByteArray &)> fn) {
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
    std::unordered_map<uint8_t, UniqueUdsServiceHandlerPtr> m_handlers;
    UniqueUdsModelPtr m_model{nullptr};
    std::shared_ptr<spdlog::logger> m_logger = Logger::get("uds-mock");
};

} // namespace doip::uds

#endif /* UDSMOCK_H */
