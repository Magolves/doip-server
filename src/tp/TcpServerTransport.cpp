#include "tp/TcpServerTransport.h"
#include "tp/TcpConnectionTransport.h"
#include "Vin.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

namespace doip {

TcpServerTransport::TcpServerTransport(bool loopback)
    : m_log(Logger::get("TcpServerTransport")) {
    m_log->debug("TcpServerTransport created (loopback={})", loopback);
}

TcpServerTransport::~TcpServerTransport() {
    closeSocket();
}

void TcpServerTransport::closeSocket() {
    bool expected = true;
    if (m_isActive.compare_exchange_strong(expected, false)) {
        if (m_log) m_log->info("Closing TCP server transport (destructor)");

            m_tcpServerSocket.close();
    }
}

bool TcpServerTransport::setup(uint16_t port) {
    m_port = port;
    m_log->info("Setting up TCP server transport on port {}", port);

    if (!setupTcpSocket()) {
        m_log->error("Failed to setup TCP socket");
        return false;
    }

    m_isActive = true;
    m_log->info("TCP server transport ready on port {}", port);
    return true;
}

bool TcpServerTransport::setupTcpSocket() {
    m_log->debug("Setting up TCP server socket");

    int tmpSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (tmpSocket < 0) {
        m_log->error("Failed to create TCP socket: {}", strerror(errno));
        return false;
    }

    // Allow socket reuse
    int reuse = 1;
    if (setsockopt(tmpSocket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        m_log->warn("Failed to set SO_REUSEADDR: {}", strerror(errno));
    }

    // Bind to port
    m_serverAddress.sin_family = AF_INET;
    m_serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    m_serverAddress.sin_port = htons(m_port);

    if (bind(tmpSocket, reinterpret_cast<const struct sockaddr *>(&m_serverAddress), sizeof(m_serverAddress)) < 0) {
        m_log->error("Failed to bind TCP socket to port {}: {}", m_port, strerror(errno));
        ::close(tmpSocket);
        return false;
    }

    // Set non-blocking
    int flags = fcntl(tmpSocket, F_GETFL, 0);
    fcntl(tmpSocket, F_SETFL, flags | O_NONBLOCK);

    // Start listening
    if (listen(tmpSocket, 5) < 0) {
        m_log->error("Failed to listen on TCP socket: {}", strerror(errno));
        ::close(tmpSocket);
        return false;
    }

    m_tcpServerSocket.reset(tmpSocket);

    m_log->info("TCP server socket listening on port {}", m_port);
    return true;
}

std::unique_ptr<IConnectionTransport> TcpServerTransport::acceptConnection() {
    if (!m_isActive || m_tcpServerSocket.get() < 0) {
        return nullptr;
    }

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_socket = accept(m_tcpServerSocket.get(), reinterpret_cast<struct sockaddr *>(&client_addr), &client_len);

    if (client_socket < 0) {
        if (errno == EAGAIN /* || errno == EWOULDBLOCK */) {
            // No connection available (non-blocking mode)
            return nullptr;
        }
        m_log->error("Failed to accept connection: {}", strerror(errno));
        return nullptr;
    }

    // Log accepted connection
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
    m_log->info("Accepted connection from {}:{}", client_ip, ntohs(client_addr.sin_port));

    return std::make_unique<TcpConnectionTransport>(client_socket);
}


void TcpServerTransport::close() {
    closeSocket();
}

bool TcpServerTransport::isActive() const {
    return m_isActive.load();
}

std::string TcpServerTransport::getIdentifier() const {
    std::ostringstream oss;
    oss << "TCP-Server:0.0.0.0:" << m_port;
    return oss.str();
}

} // namespace doip
