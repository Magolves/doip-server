#include "DoIPClient.h"
#include "DoIPMessage.h"
#include "DoIPTimes.h"
#include "DoIPPayloadType.h"
#include "util/Logger.h"
#include <cerrno>  // for errno
#include <cstring> // for strerror
#include <thread>

using namespace doip;

// -------------------------------------------------------------------
// UDP Connection and Thread Handling
// -------------------------------------------------------------------

void DoIPClient::startUdpConnection() {

    int tmpUdpSocket = socket(AF_INET, SOCK_DGRAM, 0);

    if (tmpUdpSocket >= 0) {
        m_udpSocket.reset(tmpUdpSocket);
        m_log->info("Client-UDP-Socket created successfully");

        m_serverAddress.sin_family = AF_INET;
        m_serverAddress.sin_port = htons(DOIP_UDP_DISCOVERY_PORT); // 13400
        m_serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);

        m_clientAddress.sin_family = AF_INET;
        m_clientAddress.sin_port = htons(DOIP_UDP_DISCOVERY_PORT);
        m_clientAddress.sin_addr.s_addr = htonl(INADDR_ANY);

        // binds the socket to any IP DoIPAddress and the Port Number 13400
        auto rc = bind(m_udpSocket.get(), reinterpret_cast<struct sockaddr *>(&m_clientAddress), sizeof(m_clientAddress));
        if (!rc) {
            m_log->error("Bind failed: {}", strerror(errno));
            close(m_udpSocket.get());
        }
    }
}

void DoIPClient::startAnnouncementListener() {
    int tmpUdpAnnouncementSocket = socket(AF_INET, SOCK_DGRAM, 0);

    if (tmpUdpAnnouncementSocket >= 0) {
        m_udpVehicleAnnSocket.reset(tmpUdpAnnouncementSocket);
        m_log->info("Client-Announcement-Socket created successfully");

        // Allow socket reuse for broadcast
        int reuse = 1;
        setsockopt(m_udpVehicleAnnSocket.get(), SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        // Enable broadcast reception
        int broadcast = 1;
        if (setsockopt(m_udpVehicleAnnSocket.get(), SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
            m_log->error("Failed to enable broadcast reception: {}", strerror(errno));
        } else {
            m_log->info("Broadcast reception enabled for announcements");
        }

        m_announcementAddress.sin_family = AF_INET;
        m_announcementAddress.sin_port = htons(DOIP_UDP_TEST_EQUIPMENT_REQUEST_PORT); // Port 13401
        m_announcementAddress.sin_addr.s_addr = htonl(INADDR_ANY);

        // Bind to port 13401 for Vehicle Announcements
        if (bind(m_udpVehicleAnnSocket.get(), reinterpret_cast<struct sockaddr *>(&m_announcementAddress), sizeof(m_announcementAddress)) < 0) {
            m_log->error("Failed to bind announcement socket to port {}: {}", DOIP_UDP_TEST_EQUIPMENT_REQUEST_PORT, strerror(errno));
        } else {
            m_log->info("Announcement socket bound to port {} successfully", DOIP_UDP_TEST_EQUIPMENT_REQUEST_PORT);
        }
    } else {
        m_log->error("Failed to create announcement socket: {}", strerror(errno));
    }
}

void DoIPClient::udpThreadFunction() {
    while (m_udpRunning.load()) {
        receiveUdpMessage();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void DoIPClient::closeUdpConnection() {
    m_udpSocket.close();
    if (m_udpVehicleAnnSocket.get() >= 0) {
        m_udpVehicleAnnSocket.close();
    }
}

void DoIPClient::receiveUdpMessage() {

    unsigned int length = sizeof(m_clientAddress);

    // Set socket to timeout after 3 seconds
    struct timeval timeout;
    timeout.tv_sec = static_cast<time_t>(doip::times::client::UdpMessageTimeout.count() / 1000);
    timeout.tv_usec = 0;
    setsockopt(m_udpSocket.get(), SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    int bytesRead;
    bytesRead = recvfrom(m_udpSocket.get(), m_receiveBuf.data(), DOIP_MAXIMUM_MTU, 0, reinterpret_cast<struct sockaddr *>(&m_clientAddress), &length);

    if (bytesRead < 0) {
        if (errno == EAGAIN) {
            m_log->warn("Timeout waiting for UDP response");
        } else {
            m_log->error("Error receiving UDP message: {}", strerror(errno));
        }
        return;
    }

    m_log->info("Received {} bytes from UDP", bytesRead);

    auto optMmsg = DoIPMessage::tryParse(m_receiveBuf.data(), static_cast<size_t>(bytesRead));
    if (!optMmsg.has_value()) {
        m_log->error("Failed to parse DoIP message from UDP data");
        return;
    }

    DoIPMessage msg = optMmsg.value();

    m_log->info("RX: {}", fmt::streamed(msg));
}

bool DoIPClient::receiveVehicleAnnouncement() {
    unsigned int length = sizeof(m_announcementAddress);
    int bytesRead;

    m_log->debug("Listening for Vehicle Announcements on port {}", DOIP_UDP_TEST_EQUIPMENT_REQUEST_PORT);

    // Set socket timeout for announcement reception
    struct timeval timeout;
    timeout.tv_sec = 5; // increase to 5 seconds for robustness in CI
    timeout.tv_usec = 0;
    setsockopt(m_udpVehicleAnnSocket.get(), SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    bytesRead = recvfrom(m_udpVehicleAnnSocket.get(), m_receiveBuf.data(), DOIP_MAXIMUM_MTU, 0,
                         reinterpret_cast<struct sockaddr *>(&m_announcementAddress), &length);
    if (bytesRead < 0) {
        if (errno == EAGAIN) {
            m_log->warn("Timeout waiting for Vehicle Announcement");
        } else {
            m_log->error("Error receiving Vehicle Announcement: {}", strerror(errno));
        }
        return false;
    }

    auto optMsg = DoIPMessage::tryParse(m_receiveBuf.data(), static_cast<size_t>(bytesRead));
    if (!optMsg.has_value()) {
        m_log->error("Failed to parse Vehicle Announcement message");
        return false;
    }

    DoIPMessage msg = optMsg.value();
    // Parse and display the announcement information
    if (msg.getPayloadType() == DoIPPayloadType::VehicleIdentificationResponse) {
        m_log->info("Vehicle Announcement received: {}", fmt::streamed(msg));
        parseVehicleIdentificationResponse(msg);
        return true;
    }
    return false;
}

ssize_t DoIPClient::sendVehicleIdentificationRequest(const char *inet_address) {

    int setAddressError = inet_aton(inet_address, &(m_serverAddress.sin_addr));

    if (setAddressError != 0) {
        m_log->info("Address set successfully");
    } else {
        m_log->error("Could not set address. Try again");
    }

    int socketError = setsockopt(m_udpSocket.get(), SOL_SOCKET, SO_BROADCAST, &m_broadcast, sizeof(m_broadcast));

    if (socketError == 0) {
        m_log->info("Broadcast Option set successfully");
    }

    DoIPMessage vehicleIdReq = message::makeVehicleIdentificationRequest();

    ssize_t bytesSent = sendto(m_udpSocket.get(), vehicleIdReq.data(), vehicleIdReq.size(), 0, reinterpret_cast<struct sockaddr *>(&m_serverAddress), sizeof(m_serverAddress));
    m_log->info("Sent Vehicle Identification Request to {}:{}", inet_address, ntohs(m_serverAddress.sin_port));

    if (bytesSent > 0) {
        m_log->info("Sending Vehicle Identification Request");
    }

    return bytesSent;
}

// -------------------------------------------------------------------
// TCP Connection and Thread Handling
// -------------------------------------------------------------------

bool DoIPClient::startTcpConnection(const char *inet_address, uint16_t port) {
    int tmpSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (tmpSocket >= 0) {
        m_tcpSocket.reset(tmpSocket);
        m_log->info("Client TCP-Socket created successfully");

        bool connectedFlag = false;
        const char *ipAddr = inet_address;
        m_serverAddress.sin_family = AF_INET;
        m_serverAddress.sin_port = htons(port);
        inet_aton(ipAddr, &(m_serverAddress.sin_addr));

        int retries = 3;
        while (!connectedFlag && retries > 0) {
            int tmpConnSocket = connect(m_tcpSocket.get(), reinterpret_cast<struct sockaddr *>(&m_serverAddress), sizeof(m_serverAddress));
            if (tmpConnSocket != -1) {
                m_tcpClientSocket.reset(tmpConnSocket);
                connectedFlag = true;
                m_log->info("Connection to server established");

                if (!activateRouting()) {
                    m_log->error("Routing activation failed - connection closed");
                    m_tcpClientSocket.close();
                    m_model->routingActivationFinished(*this, false, m_logicalAddress);
                    return false;
                }

                m_model->routingActivationFinished(*this, true, m_logicalAddress);

                m_tcpRunning.store(true);
                m_tcpThread = std::thread(&DoIPClient::tcpThreadFunction, this);
                return true;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
            retries--;
        }
    }

    return false;
}

void DoIPClient::tcpThreadFunction() {
    int sendRetries = 5;
    int receiveRetries = 5;

    while (m_tcpRunning.load()) {
        if (m_messageQueue.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        DoIPMessage msg;
        m_messageQueue.pop(msg);
        if (sendDoIPMessage(msg) < 0) {
            --sendRetries;
            if (sendRetries == 0) {
                m_log->error("Exceeded maximum send retries, close connection");
                m_tcpRunning.store(false);
                break;
            }
            m_log->error("Failed to send DoIP message from queue, retries left: {}", sendRetries);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        } else {
            m_model->messageSent(*this, msg);
        }

        sendRetries = 5;

        // Receive response (ack or alive check)
        auto optAck = receiveMessage();
        if (optAck == std::nullopt) {
            m_log->error("Failed to receive DoIP ack");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        } else {
            reactToMessage(optAck.value());
        }

        if (m_receiveState == ReceiveState::Quit) {
            m_log->info("Receive state set to Quit, closing TCP thread");
            break;
        }

        if (m_receiveState == ReceiveState::WaitForAckOrAliveCheck) {
            continue;
        }

        auto optMsg = receiveMessage();
        if (optMsg == std::nullopt) {
            --receiveRetries;
            if (receiveRetries == 0) {
                m_log->error("Exceeded maximum receive retries, close connection");
                m_tcpRunning.store(false);
                break;
            }
            m_log->error("Failed to receive DoIP message, retries left: {}", receiveRetries);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        } else {
            receiveRetries = 5;
            reactToMessage(optMsg.value());
        }
    }
}

void DoIPClient::reactToMessage(const DoIPMessage &msg) {
    if (m_receiveState == ReceiveState::WaitForAckOrAliveCheck) {
        switch (msg.getPayloadType()) {
        case DoIPPayloadType::AliveCheckRequest:
            m_log->info("Received Alive Check Request, sending Alive Check Response");
            if (sendAliveCheckResponse() <= 0) {
                m_log->error("Failed to send Alive Check Response");
                m_model->error(*this, "Failed to send Alive Check Response");
            }
            break;
        case DoIPPayloadType::DiagnosticMessageNegativeAck:
            m_model->diagMessageAcked(*this, msg.getDiagnosticAck().value_or(DoIPDiagnosticAck::TransportProtocolError));
            break;
        case DoIPPayloadType::DiagnosticMessageAck:
            // received diag msg ack -> proceed to receive diag messages
            updateReceiveState(ReceiveState::WaitForDiagnosticMessage);
            m_model->diagMessageAcked(*this, DoIPDiagnosticAck::PositiveAck);
            break;
        default:
            m_log->warn("Received unexpected message type ({}) while waiting for Ack or Alive Check", fmt::streamed(msg.getPayloadType()));
            m_model->error(*this, "Received unexpected message type");
            break;
        }
    } else if (m_receiveState == ReceiveState::WaitForDiagnosticMessage) {
        // Process diagnostic message
        auto newState = ReceiveState::WaitForAckOrAliveCheck;
        auto payloadType = msg.getPayloadType();
        switch (payloadType) {
        case DoIPPayloadType::DiagnosticMessage: {
            auto result = m_model->diagMessageReceived(*this, msg);
            if (result == CallbackResult::Stop) {
                newState = ReceiveState::Quit;
                m_tcpRunning.store(false);
            }
            break;
        }
        default:
            m_log->warn("Received unhandled DoIP message type: {}", fmt::streamed(payloadType));
            m_model->error(*this, "Received unexpected message type");
            break;
        }
        updateReceiveState(newState);
    }

}

bool DoIPClient::activateRouting() {
    ssize_t result = sendRoutingActivationRequest();
    if (result < 0) {
        m_log->error("Failed to send Routing Activation Request: {}", strerror(errno));
        return false;
    }

    auto optMsg = receiveMessage();
    if (optMsg == std::nullopt) {
        m_log->error("Failed to receive Routing Activation Response");
        return false;
    }

    DoIPMessage msg = optMsg.value();
    if (msg.getPayloadType() != DoIPPayloadType::RoutingActivationResponse) {
        m_log->error("Received unexpected message type ({}) instead of Routing Activation Response", fmt::streamed(msg.getPayloadType()));
        return false;
    }

    auto optLogicalAddress = msg.getSourceAddress();
    if (!optLogicalAddress) {
        m_log->error("Routing Activation Response missing logical address");
        return false;
    }

    m_logicalAddress = optLogicalAddress.value();

    return true;
}

void DoIPClient::closeTcpConnection() {
    m_tcpRunning.store(false);

    // Only join if we're not being called from the TCP thread itself
    // This prevents deadlock when called from within a callback
    if (m_tcpThread.get_id() != std::this_thread::get_id() && m_tcpThread.joinable()) {
        m_tcpThread.join();
    }

    m_tcpClientSocket.close();
    m_tcpSocket.close();
}


bool DoIPClient::reconnectServer() {
    closeTcpConnection();
    return startTcpConnection();
}

void DoIPClient::sendMessage(const DoIPMessage &msg) {
    m_messageQueue.push(msg);
}

ssize_t DoIPClient::sendDoIPMessage(const DoIPMessage &msg) {
    m_log->info("TX: {}", fmt::streamed(msg));
    return write(m_tcpSocket.get(), msg.data(), msg.size());
}

ssize_t DoIPClient::sendRoutingActivationRequest() {
    return sendDoIPMessage(message::makeRoutingActivationRequest(m_sourceAddress));
}

void DoIPClient::sendDiagnosticMessage(const ByteArray &payload) {
    sendMessage(message::makeDiagnosticMessage(m_sourceAddress, m_logicalAddress, payload));
}

ssize_t DoIPClient::sendAliveCheckResponse() {
    return sendDoIPMessage(message::makeAliveCheckResponse(m_sourceAddress));
}

/*
 * Receive a message from server
 */
std::optional<DoIPMessage> DoIPClient::receiveMessage() {

    ssize_t bytesRead = recv(m_tcpSocket.get(), m_receiveBuf.data(), DOIP_MAXIMUM_DIAG_PAYLOAD, 0);

    if (bytesRead < 0) {
        m_log->error("Error receiving data from server");
        return std::nullopt;
    }

    if (!bytesRead) // if server is disconnected from client; client gets empty messages
    {
        emptyMessageCounter++;

        if (emptyMessageCounter == 5) {
            m_log->warn("Received too many empty messages. Reconnect TCP connection");
            emptyMessageCounter = 0;
            if (!reconnectServer()) {
                m_log->error("Reconnection failed");
            }
        }
        return std::nullopt;
    }

    auto optMmsg = DoIPMessage::tryParse(m_receiveBuf.data(), static_cast<size_t>(bytesRead));
    if (!optMmsg.has_value()) {
        m_log->error("Failed to parse DoIP message from received data");
        return std::nullopt;
    }

    DoIPMessage msg = optMmsg.value();
    m_log->info("RX: {}", fmt::streamed(msg));
    return msg;
}


/**
 * Sets the source address for this client
 * @param address   source address for the client
 */
void DoIPClient::setSourceAddress(const DoIPAddress &address) {
    m_sourceAddress = address;
}

void DoIPClient::parseVehicleIdentificationResponse(const DoIPMessage &msg) {
    auto optVin = msg.getVin();
    auto optEid = msg.getEid();
    auto optGid = msg.getGid();
    auto optLogicalAddress = msg.getLogicalAddress();
    auto optFurtherAction = msg.getFurtherActionRequest();

    if (!optVin || !optEid || !optGid || !optLogicalAddress || !optFurtherAction) {
        m_log->warn("Incomplete Vehicle Identification Response received: Missing VIN, EID, GID, Logical Address or Further Action Request");
    }

    m_vin = optVin.value();
    m_eid = optEid.value();
    m_gid = optGid.value();
    m_logicalAddress = optLogicalAddress.value();
    m_furtherActionReqResult = optFurtherAction.value();
}

void DoIPClient::printVehicleInformationResponse() {
    m_log->info("Vehicle Identification Response:");
    m_log->info("VIN: {}", fmt::streamed(m_vin));
    m_log->info("EID: {}", fmt::streamed(m_eid));
    m_log->info("GID: {}", fmt::streamed(m_gid));
    m_log->info("Logical Address: 0x{:04X}", m_logicalAddress);
    m_log->info("Further Action Request Result: {}", static_cast<uint8_t>(m_furtherActionReqResult));
}
