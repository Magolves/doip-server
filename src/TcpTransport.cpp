#include "TcpTransport.h"
#include "DoIPMessage.h"
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cerrno>

namespace doip {

TcpTransport::TcpTransport(int socket)
    : m_socket(socket),
      m_log(Logger::get("TcpTransport")) {
    initializeIdentifier();
    m_log->debug("TcpTransport created for socket {}, identifier: {}", m_socket, m_identifier);
}

TcpTransport::~TcpTransport() {
    close();
}

void TcpTransport::initializeIdentifier() {
    struct sockaddr_in addr{};
    socklen_t addrLen = sizeof(addr);

    if (getpeername(m_socket, reinterpret_cast<struct sockaddr*>(&addr), &addrLen) == 0) {
        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr.sin_addr, ipStr, sizeof(ipStr));
        m_identifier = std::string(ipStr) + ":" + std::to_string(ntohs(addr.sin_port));
    } else {
        m_identifier = "socket_" + std::to_string(m_socket);
    }
}

ssize_t TcpTransport::sendMessage(const DoIPMessage &msg) {
    if (!m_isActive) {
        m_log->warn("Attempted to send on closed transport: {}", m_identifier);
        return -1;
    }

    ssize_t result = write(m_socket, msg.data(), msg.size());
    if (result < 0) {
        m_log->error("Failed to send {} bytes on {}: {}", msg.size(), m_identifier, strerror(errno));
        m_isActive = false;
    } else {
        m_log->debug("Sent {} bytes on {}", result, m_identifier);
    }
    return result;
}

std::optional<DoIPMessage> TcpTransport::receiveMessage() {
    if (!m_isActive) {
        m_log->warn("Attempted to receive on closed transport: {}", m_identifier);
        return std::nullopt;
    }

    // Read DoIP header (8 bytes)
    m_log->debug("Waiting for DoIP header on {}", m_identifier);
    uint8_t headerBuf[DOIP_HEADER_SIZE];
    ssize_t headerBytes = receiveExactly(headerBuf, DOIP_HEADER_SIZE);

    if (headerBytes != DOIP_HEADER_SIZE) {
        if (headerBytes == 0) {
            m_log->info("Connection closed by peer: {}", m_identifier);
        } else {
            m_log->error("Failed to receive complete header on {}: got {} of {} bytes",
                        m_identifier, headerBytes, DOIP_HEADER_SIZE);
        }
        m_isActive = false;
        return std::nullopt;
    }

    // Parse header to get payload type and length
    auto optHeader = DoIPMessage::tryParseHeader(headerBuf, DOIP_HEADER_SIZE);
    if (!optHeader.has_value()) {
        m_log->error("Invalid DoIP header received on {}", m_identifier);
        m_isActive = false;
        return std::nullopt;
    }

    auto [payloadType, payloadLength] = *optHeader;
    m_log->debug("Received header on {}: type={}, length={}", m_identifier,
                fmt::streamed(payloadType), payloadLength);

    // Read payload if present
    if (payloadLength > 0) {
        if (payloadLength > m_receiveBuffer.size()) {
            m_log->error("Payload length {} exceeds buffer size {} on {}",
                        payloadLength, m_receiveBuffer.size(), m_identifier);
            m_isActive = false;
            return std::nullopt;
        }

        m_log->debug("Waiting for {} bytes of payload on {}", payloadLength, m_identifier);
        ssize_t payloadBytes = receiveExactly(m_receiveBuffer.data(), payloadLength);

        if (payloadBytes != static_cast<ssize_t>(payloadLength)) {
            m_log->error("Failed to receive complete payload on {}: got {} of {} bytes",
                        m_identifier, payloadBytes, payloadLength);
            m_isActive = false;
            return std::nullopt;
        }
    }

    // Construct DoIPMessage
    DoIPMessage msg(payloadType, m_receiveBuffer.data(), payloadLength);
    m_log->debug("Successfully received message on {}: {}", m_identifier, fmt::streamed(msg));
    return msg;
}

ssize_t TcpTransport::receiveExactly(uint8_t *buffer, size_t length) {
    size_t totalReceived = 0;
    size_t remaining = length;

    while (remaining > 0) {
        ssize_t result = recv(m_socket, buffer + totalReceived, remaining, 0);

        if (result < 0) {
            // Handle non-blocking or interrupted system call
            if (errno == EAGAIN || errno == EINTR) {
                continue; // Retry
            }
            // Real error
            m_log->error("recv() failed on {}: {}", m_identifier, strerror(errno));
            return totalReceived;
        } else if (result == 0) {
            // Connection closed by peer
            return totalReceived;
        }

        totalReceived += static_cast<size_t>(result);
        remaining -= static_cast<size_t>(result);
    }

    return static_cast<ssize_t>(totalReceived);
}

void TcpTransport::close() {
    bool expected = true;
    if (m_isActive.compare_exchange_strong(expected, false)) {
        m_log->debug("Closing transport: {}", m_identifier);
        ::close(m_socket);
        m_socket = -1;
    }
}

bool TcpTransport::isActive() const {
    return m_isActive.load();
}

std::string TcpTransport::getIdentifier() const {
    return m_identifier;
}

} // namespace doip
