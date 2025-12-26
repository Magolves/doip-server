#include "DoIPConnection.h"
#include "DoIPMessage.h"
#include "DoIPPayloadType.h"
#include "Logger.h"
#include "tp/TcpConnectionTransport.h"

#include <iomanip>
#include <iostream>

namespace doip {

DoIPConnection::DoIPConnection(UniqueConnectionTransportPtr transport, UniqueServerModelPtr model, const SharedTimerManagerPtr<ConnectionTimers>& timerManager)
    : DoIPDefaultConnection(std::move(model), std::move(transport), timerManager) {
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

    handleMessage(message);
    return 1;
}

// === IConnectionContext interface implementation ===

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

    // Call base class to handle state machine, transport cleanup, and notification
    DoIPDefaultConnection::closeConnection(reason);
}

DoIPAddress DoIPConnection::getServerAddress() const {
    return m_serverModel->serverAddress;
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