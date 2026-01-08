#ifndef MOCKSERVERTRANSPORT_H
#define MOCKSERVERTRANSPORT_H

#include "tp/IServerTransport.h"
#include "tp/MockConnectionTransport.h"
#include "util/ThreadSafeQueue.h"
#include <atomic>
#include <memory>

namespace doip {

/**
 * @brief Mock server transport for testing
 *
 * Simulates a server transport without real sockets.
 * Allows injecting connections and inspecting broadcasts.
 */
class MockServerTransport : public IServerTransport {
  public:
    /**
     * @brief Construct a mock server transport
     *
     * @param identifier Descriptive name for logging
     */
    explicit MockServerTransport(const std::string &identifier = "mock-server");

    ~MockServerTransport() override = default;

    // IServerTransport interface
    bool setup(uint16_t port) override;
    std::unique_ptr<IConnectionTransport> acceptConnection() override;
    //--ssize_t sendBroadcast(const DoIPMessage &msg, uint16_t port) override;
    void close() override;
    bool isActive() const override;
    std::string getIdentifier() const override;

    // Testing interface

    /**
     * @brief Inject a mock connection that will be returned by acceptConnection()
     *
     * @param connection The mock connection to inject
     */
    void injectConnection(std::unique_ptr<MockConnectionTransport> connection);

    /**
     * @brief Clear all queues (connections and broadcasts)
     */
    void clearQueues();

  private:
    std::string m_identifier;
    uint16_t m_port{0};
    std::atomic<bool> m_isActive{false};

    // Queue for injected connections
    ThreadSafeQueue<std::unique_ptr<MockConnectionTransport>> m_connectionQueue;
};

} // namespace doip

#endif /* MOCKSERVERTRANSPORT_H */
