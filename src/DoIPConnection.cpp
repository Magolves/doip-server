#include "DoIPConnection.h"
#include "DoIPMessage.h"
#include "DoIPPayloadType.h"
#include "Logger.h"
#include "TcpConnectionTransport.h"

#include <iomanip>
#include <iostream>

namespace doip {

DoIPConnection::DoIPConnection(int tcpSocket, UniqueServerModelPtr model, const SharedTimerManagerPtr<ConnectionTimers>& timerManager)
    : DoIPDefaultConnection(std::move(model), std::make_unique<TcpConnectionTransport>(tcpSocket), timerManager),
      m_logicalAddress(ZERO_ADDRESS),
      m_tcpSocket(tcpSocket) {
}

/*
 * Closes the socket for this server (private method)
 */
void DoIPConnection::closeSocket() {
    m_transport->close(DoIPCloseReason::ApplicationRequest);
}

/*
 * Receives a message from the client and calls reactToReceivedTcpMessage method
 * @return      amount of bytes which were send back to client
 *              or -1 if error occurred
 */
int DoIPConnection::receiveMessage() {
    m_log->info("Waiting for DoIP Header...");
   auto optMessage = m_transport->receiveMessage();
    if (!optMessage.has_value()) {
        m_log->info("No message received (connection closed or error)");
        return -1;
    }

    const DoIPMessage &message = optMessage.value();
    m_log->info("Received DoIP message: {}", fmt::streamed(message));

    handleMessage2(message);
    return 1;
}

/**
 * Sends a message back to the connected client
 * @param message           contains generic header and payload specific content
 * @param messageLength     length of the complete message
 * @return                  number of bytes written is returned,
 *                          or -1 if error occurred
 */
ssize_t DoIPConnection::sendMessage(const uint8_t *message, size_t messageLength) {
    ssize_t result = write(m_tcpSocket, message, messageLength);
    return result;
}

// === IConnectionContext interface implementation ===
ssize_t DoIPConnection::sendProtocolMessage(const DoIPMessage &msg) {
    return DoIPDefaultConnection::sendProtocolMessage(msg);  // Delegate to base class
}

std::optional<DoIPMessage> DoIPConnection::receiveProtocolMessage() {
    return m_transport->receiveMessage();  // Delegate to transport
}

void DoIPConnection::closeConnection(DoIPCloseReason reason) {
    // Guard against recursive calls using atomic exchange
    bool expected = false;
    if (!m_isClosing.compare_exchange_strong(expected, true)) {
        m_log->debug("Connection already closing - ignoring recursive call");
        return;
    }

    m_log->info("Closing connection, reason: {}", fmt::streamed(reason));

    // Call base class to handle state machine and notification
    DoIPDefaultConnection::closeConnection(reason);

    close(m_tcpSocket);
    m_tcpSocket = 0;
}

DoIPAddress DoIPConnection::getServerAddress() const {
    return m_serverModel->serverAddress;
}

DoIPAddress DoIPConnection::getClientAddress() const {
    return m_logicalAddress;
}

void DoIPConnection::setClientAddress(const DoIPAddress &address) {
    m_logicalAddress = address;
}

DoIPDiagnosticAck DoIPConnection::notifyDiagnosticMessage(const DoIPMessage &msg) {
    // Forward to application callback
    if (m_serverModel->onDiagnosticMessage) {
        return m_serverModel->onDiagnosticMessage(*this, msg);
    }
    // Default: ACK
    return std::nullopt;
}

void DoIPConnection::notifyConnectionClosed(DoIPCloseReason reason) {
    (void)reason; // Could extend DoIPServerModel to include close reason
    if (m_serverModel->onCloseConnection) {
        m_serverModel->onCloseConnection(*this, reason);
    }
}

void DoIPConnection::notifyDiagnosticAckSent(DoIPDiagnosticAck ack) {
    if (m_serverModel->onDiagnosticNotification) {
        m_serverModel->onDiagnosticNotification(*this, ack);
    }
}

bool DoIPConnection::hasDownstreamHandler() const {
    return m_serverModel->hasDownstreamHandler();
}

} // namespace doip