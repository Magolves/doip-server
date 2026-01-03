
#ifndef DOIPCLIENT_H
#define DOIPCLIENT_H

#include "DoIPConfig.h"
#include "DoIPClientModel.h"
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

const int _maxDataSize = 64;



class DoIPClient {

  public:
    DoIPClient(UniqueDoIPClientModelPtr model = std::make_unique<DoIPClientModel>()) : m_model(std::move(model)) { m_receiveBuf.reserve(DOIP_MAXIMUM_MTU); }

    [[nodiscard]] bool startTcpConnection();

    [[nodiscard]] bool isTcpConnected() const noexcept { return m_connected.valid(); }
    [[nodiscard]] bool reconnectServer();
    void closeTcpConnection();

    void sendMessage(const DoIPMessage &msg);

    void startUdpConnection();
    void startAnnouncementListener();
    void closeUdpConnection();


    ssize_t sendRoutingActivationRequest();
    std::optional<DoIPMessage> receiveRoutingActivationResponse();

    void receiveUdpMessage();
    ssize_t sendVehicleIdentificationRequest(const char *inet_address);
    [[nodiscard]]
    bool receiveVehicleAnnouncement();

    /**
     * Sends a diagnostic message to the server
     * @param payload          data that will be given to the ecu
     */
    ssize_t sendDiagnosticMessage(const ByteArray &payload);

    /**
     * Sends a alive check response containing the clients source address to the server
     */
    ssize_t sendAliveCheckResponse();
    void setSourceAddress(const DoIPAddress &address);
    void printVehicleInformationResponse();

  private:
    UniqueDoIPClientModelPtr m_model;
    ByteArray m_receiveBuf;
    Socket m_tcpSocket, m_udpSocket, m_udpAnnouncementSocket, m_connected;
    ThreadSafeQueue<DoIPMessage> m_messageQueue;
    std::atomic<bool> m_tcpRunning{false};
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

    void tcpThreadFunction();
    bool activateRouting();
    ssize_t sendDoIPMessage(const DoIPMessage &msg);
    std::optional<DoIPMessage> receiveMessage();


    void parseVehicleIdentificationResponse(const DoIPMessage &msg);

    int emptyMessageCounter = 0;
};

} // namespace doip

#endif /* DOIPCLIENT_H */
