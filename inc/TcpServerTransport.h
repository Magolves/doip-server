#ifndef TCPSERVERTRANSPORT_H
#define TCPSERVERTRANSPORT_H

#include "IServerTransport.h"
#include "Logger.h"
#include <atomic>
#include <memory>
#include <netinet/in.h>

namespace doip {

/**
 * @brief TCP-based server transport implementation
 *
 * Manages TCP server socket (listen/accept) and UDP socket (announcements)
 */
class TcpServerTransport : public IServerTransport {
  public:
    /**
     * @brief Construct a TCP server transport
     *
     * @param loopback If true, use loopback interface; otherwise use broadcast
     */
    explicit TcpServerTransport(bool loopback = false);

    /**
     * @brief Destructor - closes sockets
     */
    ~TcpServerTransport() override;

    // Disable copy
    TcpServerTransport(const TcpServerTransport &) = delete;
    TcpServerTransport &operator=(const TcpServerTransport &) = delete;

    // IServerTransport interface
    bool setup(uint16_t port) override;
    std::unique_ptr<IConnectionTransport> acceptConnection() override;
    ssize_t sendBroadcast(const DoIPMessage &msg, uint16_t port) override;
    void close() override;
    bool isActive() const override;
    std::string getIdentifier() const override;

  private:
    int m_tcpServerSocket{-1};
    int m_udpSocket{-1};
    uint16_t m_port{0};
    bool m_loopback;
    std::atomic<bool> m_isActive{false};
    std::shared_ptr<spdlog::logger> m_log;

    struct sockaddr_in m_serverAddress{};
    struct sockaddr_in m_broadcastAddress{};

    /**
     * @brief Set up TCP server socket (bind + listen)
     */
    bool setupTcpSocket();

    /**
     * @brief Set up UDP socket for announcements
     */
    bool setupUdpSocket();

    /**
     * @brief Configure broadcast/multicast settings
     */
    void configureBroadcast();

    void closeSocket();
};

} // namespace doip

#endif /* TCPSERVERTRANSPORT_H */
