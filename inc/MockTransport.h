#ifndef MOCKTRANSPORT_H
#define MOCKTRANSPORT_H

#include "ITransport.h"
#include "ThreadSafeQueue.h"
#include <queue>
#include <atomic>
#include <string>

namespace doip {

/**
 * @brief Mock transport for testing DoIP connections without real sockets
 *
 * Uses in-memory queues for bidirectional message passing.
 * Useful for unit testing state machines and protocol logic.
 */
class MockTransport : public ITransport {
  public:
    /**
     * @brief Construct a mock transport with an identifier
     *
     * @param identifier Descriptive name for this transport (e.g., "test-client-1")
     */
    explicit MockTransport(const std::string &identifier = "mock-transport");

    ~MockTransport() override = default;

    // ITransport interface
    ssize_t sendMessage(const DoIPMessage &msg) override;
    std::optional<DoIPMessage> receiveMessage() override;
    void close() override;
    bool isActive() const override;
    std::string getIdentifier() const override;

    // Testing interface

    /**
     * @brief Inject a message into the receive queue (simulates incoming message)
     *
     * @param msg The message to inject
     */
    void injectMessage(const DoIPMessage &msg);

    /**
     * @brief Get the next sent message (simulates reading what was sent)
     *
     * @return The next sent message, or std::nullopt if queue is empty
     */
    std::optional<DoIPMessage> popSentMessage();

    /**
     * @brief Check if any messages have been sent
     *
     * @return true if sent queue is not empty
     */
    bool hasSentMessages() const;

    /**
     * @brief Get the number of messages in the sent queue
     *
     * @return Number of sent messages waiting to be read
     */
    size_t sentMessageCount() const;

    /**
     * @brief Clear all queues (sent and receive)
     */
    void clearQueues();

    /**
     * @brief Set whether receive should block or return immediately
     *
     * @param blocking If true, receiveMessage() blocks until message available
     */
    void setBlocking(bool blocking) { m_blocking = blocking; }

  private:
    std::string m_identifier;
    std::atomic<bool> m_isActive{true};
    bool m_blocking{false};

    // Queue for messages sent by the connection (outgoing)
    ThreadSafeQueue<DoIPMessage> m_sentQueue;

    // Queue for messages to be received by the connection (incoming)
    ThreadSafeQueue<DoIPMessage> m_receiveQueue;
};

} // namespace doip

#endif /* MOCKTRANSPORT_H */
