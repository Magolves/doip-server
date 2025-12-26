#ifndef ISERVERTRANSPORT_H
#define ISERVERTRANSPORT_H

#include "DoIPMessage.h"
#include "IConnectionTransport.h"
#include <memory>
#include <string>

namespace doip {

// Forward declarations
struct ServerConfig;

/**
 * @brief Server-level transport abstraction for DoIP
 *
 * This interface handles server-level operations:
 * - Setting up listener sockets (TCP, UDP)
 * - Accepting incoming connections
 * - Broadcasting UDP announcements
 * - Server lifecycle management
 *
 * Implementations: TcpServerTransport, MockServerTransport
 */
class IServerTransport {
  public:
    virtual ~IServerTransport() = default;

    /**
     * @brief Set up the transport server (bind, listen)
     *
     * For TCP: Creates server socket, binds to port, starts listening
     * For UDP: Creates UDP socket for announcements
     * For Mock: No-op or in-memory setup
     *
     * @param port The port to bind to (e.g., 13400 for TCP)
     * @return true on success, false on error
     */
    virtual bool setup(uint16_t port) = 0;

    /**
     * @brief Accept a new incoming connection (blocking or with timeout)
     *
     * For TCP: Calls accept() on server socket
     * For Mock: Returns pre-injected mock connections
     *
     * @return Unique pointer to connection transport, or nullptr if no connection available
     */
    virtual std::unique_ptr<IConnectionTransport> acceptConnection() = 0;

    /**
     * @brief Send a broadcast/announcement message (UDP)
     *
     * For TCP: Sends UDP broadcast on port 13400
     * For Mock: Stores message for inspection
     *
     * @param msg The DoIP message to broadcast (typically vehicle announcement)
     * @param port The destination port for broadcast
     * @return Number of bytes sent, or -1 on error
     */
    virtual ssize_t sendBroadcast(const DoIPMessage &msg, uint16_t port) = 0;

    /**
     * @brief Close the server transport and cleanup resources
     *
     * Closes all listening sockets and stops accepting connections
     */
    virtual void close() = 0;

    /**
     * @brief Check if the server transport is currently active
     *
     * @return true if server is listening and can accept connections
     */
    virtual bool isActive() const = 0;

    /**
     * @brief Get a human-readable identifier for this server transport
     *
     * Useful for logging (e.g., "TCP-Server:0.0.0.0:13400", "Mock-Server")
     *
     * @return Server transport identifier string
     */
    virtual std::string getIdentifier() const = 0;
};

using UniqueServerTransportPtr = std::unique_ptr<IServerTransport>;

} // namespace doip

#endif /* ISERVERTRANSPORT_H */
