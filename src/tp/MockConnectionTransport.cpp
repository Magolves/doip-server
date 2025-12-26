#include "tp/MockConnectionTransport.h"

namespace doip {

MockConnectionTransport::MockConnectionTransport(const std::string &identifier)
    : m_identifier(identifier) {
}

ssize_t MockConnectionTransport::sendMessage(const DoIPMessage &msg) {
    if (!m_isActive) {
        return -1;
    }

    m_sentQueue.push(msg);
    return static_cast<ssize_t>(msg.size());
}

std::optional<DoIPMessage> MockConnectionTransport::receiveMessage() {
    if (!m_isActive) {
        return std::nullopt;
    }

    if (m_blocking) {
        // Block until message available or transport closed
        DoIPMessage msg;
        if (m_receiveQueue.waitAndPop(msg)) {
            return msg;
        }
        return std::nullopt;
    } else {
        // Non-blocking: return immediately if no message
        DoIPMessage msg;
        if (m_receiveQueue.tryPop(msg)) {
            return msg;
        }
        return std::nullopt;
    }
}

void MockConnectionTransport::close(DoIPCloseReason reason) {
    (void)reason; // Unused in mock
    m_isActive = false;
    // Wake up any blocking waiters
    m_receiveQueue.clear();
    m_sentQueue.clear();
}

bool MockConnectionTransport::isActive() const {
    return m_isActive.load();
}

std::string MockConnectionTransport::getIdentifier() const {
    return m_identifier;
}

void MockConnectionTransport::injectMessage(const DoIPMessage &msg) {
    m_receiveQueue.push(msg);
}

std::optional<DoIPMessage> MockConnectionTransport::popSentMessage() {
    DoIPMessage msg;
    if (m_sentQueue.tryPop(msg)) {
        return msg;
    }
    return std::nullopt;
}

bool MockConnectionTransport::hasSentMessages() const {
    return !m_sentQueue.empty();
}

size_t MockConnectionTransport::sentMessageCount() const {
    return m_sentQueue.size();
}

void MockConnectionTransport::clearQueues() {
    m_sentQueue.clear();
    m_receiveQueue.clear();
}

} // namespace doip
