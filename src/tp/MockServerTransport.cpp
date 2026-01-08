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

} // namespace doip
