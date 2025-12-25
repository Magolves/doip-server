#ifndef TCPTRANSPORT_H
#define TCPTRANSPORT_H

#include "ITransport.h"
#include "Logger.h"
#include "gen/DoIPConfig.h"
#include <array>
#include <atomic>
#include <memory>
#include <arpa/inet.h>
#include <sys/socket.h>

namespace doip {

/**
 * @brief TCP-based transport implementation for DoIP
 *
 * Wraps a TCP socket and provides DoIP message send/receive functionality.
 */
class TcpTransport : public ITransport {
  public:
    /**
     * @brief Construct a TCP transport from an existing socket
     *
     * @param socket The connected TCP socket (takes ownership)
     */
    explicit TcpTransport(int socket);

    /**
     * @brief Destructor - closes the socket
     */
    ~TcpTransport() override;

    // Disable copy
    TcpTransport(const TcpTransport&) = delete;
    TcpTransport& operator=(const TcpTransport&) = delete;

    // ITransport interface
    ssize_t sendMessage(const DoIPMessage &msg) override;
    std::optional<DoIPMessage> receiveMessage() override;
    void close() override;
    bool isActive() const override;
    std::string getIdentifier() const override;

  private:
    int m_socket;
    std::array<uint8_t, DOIP_MAXIMUM_MTU> m_receiveBuffer{};
    std::atomic<bool> m_isActive{true};
    std::shared_ptr<spdlog::logger> m_log;
    std::string m_identifier;

    /**
     * @brief Receive a fixed number of bytes from the socket
     *
     * @param buffer Buffer to store received data
     * @param length Number of bytes to receive
     * @return Number of bytes received, or 0 on connection close, -1 on error
     */
    ssize_t receiveExactly(uint8_t *buffer, size_t length);

    /**
     * @brief Initialize the transport identifier string
     */
    void initializeIdentifier();
};

} // namespace doip

#endif /* TCPTRANSPORT_H */
