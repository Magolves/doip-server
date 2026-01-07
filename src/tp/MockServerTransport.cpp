#include "tp/MockServerTransport.h"

namespace doip {

MockServerTransport::MockServerTransport(const std::string &identifier)
    : m_identifier(identifier) {
}

bool MockServerTransport::setup(uint16_t port) {
    m_port = port;
    m_isActive = true;
    return true;
}

std::unique_ptr<IConnectionTransport> MockServerTransport::acceptConnection() {
    if (!m_isActive) {
        return nullptr;
    }

    std::unique_ptr<MockConnectionTransport> conn;
    if (m_connectionQueue.tryPop(conn)) {
        return conn;
    }

    return nullptr;
}

void MockServerTransport::close() {
    m_isActive = false;
    clearQueues();
}

bool MockServerTransport::isActive() const {
    return m_isActive.load();
}

std::string MockServerTransport::getIdentifier() const {
    std::ostringstream oss;
    oss << m_identifier << ":" << m_port;
    return oss.str();
}

void MockServerTransport::injectConnection(std::unique_ptr<MockConnectionTransport> connection) {
    m_connectionQueue.push(std::move(connection));
}

std::optional<DoIPMessage> MockServerTransport::popBroadcast() {
    DoIPMessage msg;
    if (m_broadcastQueue.tryPop(msg)) {
        return msg;
    }
    return std::nullopt;
}

bool MockServerTransport::hasBroadcasts() const {
    return !m_broadcastQueue.empty();
}

size_t MockServerTransport::broadcastCount() const {
    return m_broadcastQueue.size();
}

void MockServerTransport::clearQueues() {
    m_connectionQueue.clear();
    m_broadcastQueue.clear();
}

} // namespace doip
