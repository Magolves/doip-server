
#ifndef DOIPCLIENT_H
#define DOIPCLIENT_H

#include "DoIPClientModel.h"
#include "DoIPConfig.h"
#include "DoIPMessage.h"
#include "util/Logger.h"
#include "util/Socket.h"
#include "util/ThreadSafeQueue.h"

#include <arpa/inet.h>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <stdlib.h>
#include <unistd.h>

namespace doip {

class DoIPClient {
    enum class ReceiveState {
        WaitForAckOrAliveCheck,
        WaitForDiagnosticMessage,
        Quit,
    };

  public:
    DoIPClient(UniqueDoIPClientModelPtr model = std::make_unique<DoIPClientModel>()) : m_model(std::move(model)) { m_receiveBuf.reserve(DOIP_MAXIMUM_MTU); }

    [[nodiscard]] bool startTcpConnection(const char *inet_address = "127.0.0.1", uint16_t port = DOIP_TCP_DEFAULT_PORT);

    [[nodiscard]] bool isTcpRunning() const noexcept { return m_tcpRunning.load(); }

    [[nodiscard]] bool reconnectServer();
    void closeTcpConnection();

    void startUdpConnection();
    [[nodiscard]] bool isUdpRunning() const noexcept { return m_udpRunning.load(); }
    void startAnnouncementListener();
    void closeUdpConnection();

    // TODO: Make private later
    void receiveUdpMessage();
    ssize_t sendVehicleIdentificationRequest(const char *inet_address);
    [[nodiscard]]
    bool receiveVehicleAnnouncement();

    /**
     * @brief Enqueues a DoIP message to the server
     * @param msg DoIP message to send
     */
    void sendMessage(const DoIPMessage &msg);

    /**
     * @brief Enqueues a diagnostic message to the server
     * @param payload data that will be given to the ecu
     */
    void sendDiagnosticMessage(const ByteArray &payload);

    void setSourceAddress(const DoIPAddress &address);
    void printVehicleInformationResponse();

    // Request the client to quit gracefully
    void requestQuit() {
        updateReceiveState(ReceiveState::Quit);
        m_tcpRunning.store(false);
        m_udpRunning.store(false);
    }

  protected:
    void updateReceiveState(ReceiveState newState) {
        if (m_receiveState != newState) {
            m_log->info("Receive state changed from {} to {}", static_cast<int>(m_receiveState), static_cast<int>(newState));
            m_receiveState = newState;
        }
    }

  private:
    UniqueDoIPClientModelPtr m_model;
    ByteArray m_receiveBuf;
    Socket m_tcpSocket, m_tcpClientSocket;
    Socket m_udpSocket, m_udpVehicleAnnSocket;
    ThreadSafeQueue<DoIPMessage> m_messageQueue;
    std::atomic<bool> m_tcpRunning{false};
    std::atomic<bool> m_udpRunning{false};
    std::thread m_tcpThread{};
    int m_broadcast = 1;
    struct sockaddr_in m_serverAddress, m_clientAddress, m_announcementAddress;

    std::shared_ptr<spdlog::logger> m_log = spdlog::stdout_color_mt("doip-client");

    DoIPAddress m_sourceAddress = DoIPAddress(0xE000);
    Vin m_vin{0};
    DoIPAddress m_logicalAddress = ZERO_ADDRESS;
    EntityId m_eid{0};
    GroupId m_gid{0};
    DoIPFurtherAction m_furtherActionReqResult = DoIPFurtherAction::NoFurtherAction;

    ReceiveState m_receiveState = ReceiveState::WaitForAckOrAliveCheck;

    ssize_t sendRoutingActivationRequest();
    std::optional<DoIPMessage> receiveRoutingActivationResponse();

    /**
     * Sends a alive check response containing the clients source address to the server
     */
    ssize_t sendAliveCheckResponse();

    void tcpThreadFunction();
    [[nodiscard]] bool activateRouting();
    [[nodiscard]] ssize_t sendDoIPMessage(const DoIPMessage &msg);
    [[nodiscard]] std::optional<DoIPMessage> receiveMessage();

    void reactToMessage(const DoIPMessage &msg);

    void udpThreadFunction();
    void parseVehicleIdentificationResponse(const DoIPMessage &msg);

    int emptyMessageCounter = 0;
};

} // namespace doip

#endif /* DOIPCLIENT_H */
