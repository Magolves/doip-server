#ifndef TCPCONNECTIONTRANSPORT_H
#define TCPCONNECTIONTRANSPORT_H

#include "IConnectionTransport.h"
#include "Logger.h"
#include "gen/DoIPConfig.h"
#include <array>
#include <atomic>
#include <memory>

namespace doip {

/**
 * @brief TCP connection transport for a single client
 *
 * Wraps a connected TCP socket and provides DoIP message send/receive
 */
class TcpConnectionTransport : public IConnectionTransport {
  public:
    /**
     * @brief Construct a TCP connection transport from an existing socket
     *
     * @param socket The connected TCP socket (takes ownership)
     */
    explicit TcpConnectionTransport(int socket);

    /**
     * @brief Destructor - closes the socket
     */
    ~TcpConnectionTransport() override;

    // Disable copy
    TcpConnectionTransport(const TcpConnectionTransport &) = delete;
    TcpConnectionTransport &operator=(const TcpConnectionTransport &) = delete;

    // IConnectionTransport interface
    ssize_t sendMessage(const DoIPMessage &msg) override;
    std::optional<DoIPMessage> receiveMessage() override;
    void close(DoIPCloseReason reason) override;
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

#endif /* TCPCONNECTIONTRANSPORT_H */
