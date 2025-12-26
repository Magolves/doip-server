#ifndef MOCKSERVERTRANSPORT_H
#define MOCKSERVERTRANSPORT_H

#include "tp/IServerTransport.h"
#include "tp/MockConnectionTransport.h"
#include "ThreadSafeQueue.h"
#include <atomic>
#include <memory>
#include <queue>
#include <vector>

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
    ssize_t sendBroadcast(const DoIPMessage &msg, uint16_t port) override;
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
     * @brief Get the next broadcast message that was sent
     *
     * @return The broadcast message, or std::nullopt if queue is empty
     */
    std::optional<DoIPMessage> popBroadcast();

    /**
     * @brief Check if any broadcast messages have been sent
     *
     * @return true if broadcast queue is not empty
     */
    bool hasBroadcasts() const;

    /**
     * @brief Get the number of broadcast messages in the queue
     *
     * @return Number of broadcasts waiting to be read
     */
    size_t broadcastCount() const;

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

    // Queue for broadcast messages
    ThreadSafeQueue<DoIPMessage> m_broadcastQueue;
};

} // namespace doip

#endif /* MOCKSERVERTRANSPORT_H */
