#include "DoIPConnection.h"
#include "DoIPMessage.h"
#include "DoIPPayloadType.h"
#include "Logger.h"
#include "TcpTransport.h"

#include <iomanip>
#include <iostream>

namespace doip {

DoIPConnection::DoIPConnection(int tcpSocket, UniqueServerModelPtr model, const SharedTimerManagerPtr<ConnectionTimers>& timerManager)
    : DoIPDefaultConnection(std::move(model), std::make_unique<TcpTransport>(tcpSocket), timerManager),
      m_logicalAddress(ZERO_ADDRESS),
      m_tcpSocket(tcpSocket) {
}

/*
 * Closes the socket for this server (private method)
 */
void DoIPConnection::closeSocket() {
    closeConnection(DoIPCloseReason::ApplicationRequest);
}

/*
 * Receives a message from the client and calls reactToReceivedTcpMessage method
 * @return      amount of bytes which were send back to client
 *              or -1 if error occurred
 */
int DoIPConnection::receiveTcpMessage() {
    m_log->info("Waiting for DoIP Header...");
    uint8_t genericHeader[DOIP_HEADER_SIZE];
    unsigned int readBytes = receiveFixedNumberOfBytesFromTCP(genericHeader, DOIP_HEADER_SIZE);
    if (readBytes == DOIP_HEADER_SIZE /*&& !m_aliveCheckTimer.hasTimeout()*/) {
        m_log->info("Received DoIP Header.");

        auto optHeader = DoIPMessage::tryParseHeader(genericHeader, DOIP_HEADER_SIZE);
        if (!optHeader.has_value()) {
            // m_stateMachine.processEvent(DoIPServerEvent::InvalidMessage);
            //  TODO: Notify application of invalid message?
            m_log->error("DoIP message header parsing failed");
            closeSocket();
            return -2;
        }

        auto plType = optHeader->first;
        auto payloadLength = optHeader->second;

        m_log->info("Payload Type: {}, length: {} ", fmt::streamed(plType), payloadLength);

        if (payloadLength > 0) {
            m_log->debug("Waiting for {} bytes of payload...", payloadLength);
            unsigned int receivedPayloadBytes = receiveFixedNumberOfBytesFromTCP(m_receiveBuf.data(), payloadLength);
            if (receivedPayloadBytes < payloadLength) {
                m_log->error("DoIP message incomplete");
                // m_stateMachine.processEvent(DoIPServerEvent::InvalidMessage);
                //  todo: Notify application of invalid message?
                closeSocket();
                return -2;
            }

            DoIPMessage msg = DoIPMessage(plType, m_receiveBuf.data(), receivedPayloadBytes);
            m_log->info("RX: {}", fmt::streamed(msg));
        }

        DoIPMessage message(plType, m_receiveBuf.data(), payloadLength);
        handleMessage2(message);

        return 1;
    } else {
        closeSocket();
        return 0;
    }
    return -1;
}

/**
 * Receive exactly payloadLength bytes from the TCP stream and put them into receivedData.
 * The method blocks until receivedData bytes are received or the socket is closed.
 *
 * The parameter receivedData needs to point to a readily allocated array with
 * at least payloadLength items.
 *
 * @return number of bytes received
 */
size_t DoIPConnection::receiveFixedNumberOfBytesFromTCP(uint8_t *receivedData, size_t payloadLength) {
    size_t payloadPos = 0;
    size_t remainingPayload = payloadLength;

    while (remainingPayload > 0) {
        ssize_t result = recv(m_tcpSocket, &receivedData[payloadPos], remainingPayload, 0);
        if (result < 0) {
            // Handle non-blocking socket or interrupted system call
            if (errno == EAGAIN /*|| errno == EWOULDBLOCK */ || errno == EINTR) {
                // No data available yet or interrupted - retry
                continue;
            }
            // Real error occurred
            m_log->error("recv() failed: {}", strerror(errno));
            return payloadPos;
        } else if (result == 0) {
            // Connection closed by peer
            m_log->info("Connection closed by peer during receive");
            return payloadPos;
        }
        size_t readBytes = static_cast<size_t>(result);
        payloadPos += readBytes;
        remainingPayload -= readBytes;
    }

    return payloadPos;
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
    ssize_t sentBytes = sendMessage(msg.data(), msg.size());
    if (sentBytes < 0) {
        m_log->error("Error sending message to client: {}", fmt::streamed(msg));
    } else {
        m_log->info("Sent {} bytes to client: {}", sentBytes, fmt::streamed(msg));
    }
    return sentBytes;
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