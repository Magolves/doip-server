#include "MockTransport.h"
#include "Logger.h"

namespace doip {

MockTransport::MockTransport(const std::string &identifier)
    : m_identifier(identifier), m_log(Logger::get("mockTp")) {
}

ssize_t MockTransport::sendMessage(const DoIPMessage &msg) {
    if (!m_isActive) {
        return -1;
    }

    m_sentQueue.push(msg);
    return static_cast<ssize_t>(msg.size());
}

std::optional<DoIPMessage> MockTransport::receiveMessage() {
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

void MockTransport::close(DoIPCloseReason reason) {
    m_log->debug("Closing MockTransport: {} ({})", m_identifier, fmt::streamed(reason));
    m_isActive = false;
    // Wake up any blocking waiters
    m_receiveQueue.clear();
    m_sentQueue.clear();
}

bool MockTransport::isActive() const {
    return m_isActive.load();
}

std::string MockTransport::getIdentifier() const {
    return m_identifier;
}

void MockTransport::injectMessage(const DoIPMessage &msg) {
    m_receiveQueue.push(msg);
}

std::optional<DoIPMessage> MockTransport::popSentMessage() {
    DoIPMessage msg;
    if (m_sentQueue.tryPop(msg)) {
        return msg;
    }
    return std::nullopt;
}

bool MockTransport::hasSentMessages() const {
    return !m_sentQueue.empty();
}

size_t MockTransport::sentMessageCount() const {
    return m_sentQueue.size();
}

void MockTransport::clearQueues() {
    m_sentQueue.clear();
    m_receiveQueue.clear();
}

} // namespace doip
