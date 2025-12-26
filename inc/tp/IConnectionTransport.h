#ifndef ICONNECTIONTRANSPORT_H
#define ICONNECTIONTRANSPORT_H

#include "DoIPCloseReason.h"
#include "DoIPMessage.h"
#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace doip {

/**
 * @brief Per-connection transport abstraction for DoIP
 *
 * This interface handles communication for a single client connection:
 * - Sending DoIP messages to the client
 * - Receiving DoIP messages from the client
 * - Closing the connection
 *
 * This is the interface used by DoIPConnection/DoIPDefaultConnection
 * for actual message exchange with a specific client.
 *
 * Implementations: TcpConnectionTransport, MockConnectionTransport
 */
class IConnectionTransport {
  public:
    virtual ~IConnectionTransport() = default;

    /**
     * @brief Send a DoIP message over this connection
     *
     * @param msg The DoIP message to send
     * @return Number of bytes sent, or -1 on error
     */
    virtual ssize_t sendMessage(const DoIPMessage &msg) = 0;

    /**
     * @brief Receive a DoIP message from this connection
     *
     * This method should block until a complete DoIP message is received
     * or an error occurs.
     *
     * @return The received DoIP message, or std::nullopt on error/disconnection
     */
    virtual std::optional<DoIPMessage> receiveMessage() = 0;

    /**
     * @brief Close this connection
     *
     * @param reason Why the connection is being closed
     */
    virtual void close(DoIPCloseReason reason) = 0;

    /**
     * @brief Check if this connection is currently active
     *
     * @return true if the connection can send/receive, false otherwise
     */
    virtual bool isActive() const = 0;

    /**
     * @brief Get a human-readable identifier for this connection
     *
     * Useful for logging (e.g., "TCP:192.168.1.100:54321", "Mock-Client-1")
     *
     * @return Connection identifier string
     */
    virtual std::string getIdentifier() const = 0;
};

using UniqueConnectionTransportPtr = std::unique_ptr<IConnectionTransport>;

} // namespace doip

#endif /* ICONNECTIONTRANSPORT_H */
