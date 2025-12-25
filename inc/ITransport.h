#ifndef ITRANSPORT_H
#define ITRANSPORT_H

#include "DoIPMessage.h"
#include <cstddef>
#include <optional>

namespace doip {

/**
 * @brief Transport layer abstraction for DoIP connections
 *
 * This interface abstracts the underlying transport mechanism (TCP, mock, etc.)
 * from the DoIP protocol state machine. It provides methods for sending/receiving
 * DoIP messages and managing the transport connection lifecycle.
 */
class ITransport {
  public:
    virtual ~ITransport() = default;

    /**
     * @brief Send a DoIP message over the transport
     *
     * @param msg The DoIP message to send
     * @return Number of bytes sent, or -1 on error
     */
    virtual ssize_t sendMessage(const DoIPMessage &msg) = 0;

    /**
     * @brief Receive a DoIP message from the transport
     *
     * This method should block until a complete DoIP message is received
     * or an error occurs.
     *
     * @return The received DoIP message, or std::nullopt on error/disconnection
     */
    virtual std::optional<DoIPMessage> receiveMessage() = 0;

    /**
     * @brief Close the transport connection
     *
     * After calling this, the transport is no longer usable.
     */
    virtual void close() = 0;

    /**
     * @brief Check if the transport is currently active/connected
     *
     * @return true if the transport can send/receive, false otherwise
     */
    virtual bool isActive() const = 0;

    /**
     * @brief Get a human-readable identifier for this transport
     *
     * Useful for logging (e.g., "TCP:127.0.0.1:13400", "Mock")
     *
     * @return Transport identifier string
     */
    virtual std::string getIdentifier() const = 0;
};

using UniqueTransportPtr = std::unique_ptr<ITransport>;
} // namespace doip

#endif /* ITRANSPORT_H */
