#include "TcpServerTransport.h"
#include "TcpConnectionTransport.h"
#include "DoIPIdentifiers.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

namespace doip {

TcpServerTransport::TcpServerTransport(bool loopback)
    : m_loopback(loopback),
      m_log(Logger::get("TcpServerTransport")) {
    m_log->debug("TcpServerTransport created (loopback={})", loopback);
}

TcpServerTransport::~TcpServerTransport() {
    close();
}

bool TcpServerTransport::setup(uint16_t port) {
    m_port = port;
    m_log->info("Setting up TCP server transport on port {}", port);

    if (!setupTcpSocket()) {
        m_log->error("Failed to setup TCP socket");
        return false;
    }

    if (!setupUdpSocket()) {
        m_log->error("Failed to setup UDP socket");
        ::close(m_tcpServerSocket);
        m_tcpServerSocket = -1;
        return false;
    }

    configureBroadcast();
    m_isActive = true;
    m_log->info("TCP server transport ready on port {}", port);
    return true;
}

bool TcpServerTransport::setupTcpSocket() {
    m_log->debug("Setting up TCP server socket");

    m_tcpServerSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_tcpServerSocket < 0) {
        m_log->error("Failed to create TCP socket: {}", strerror(errno));
        return false;
    }

    // Allow socket reuse
    int reuse = 1;
    if (setsockopt(m_tcpServerSocket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        m_log->warn("Failed to set SO_REUSEADDR: {}", strerror(errno));
    }

    // Bind to port
    m_serverAddress.sin_family = AF_INET;
    m_serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    m_serverAddress.sin_port = htons(m_port);

    if (bind(m_tcpServerSocket, reinterpret_cast<const struct sockaddr *>(&m_serverAddress), sizeof(m_serverAddress)) < 0) {
        m_log->error("Failed to bind TCP socket to port {}: {}", m_port, strerror(errno));
        ::close(m_tcpServerSocket);
        m_tcpServerSocket = -1;
        return false;
    }

    // Set non-blocking
    int flags = fcntl(m_tcpServerSocket, F_GETFL, 0);
    fcntl(m_tcpServerSocket, F_SETFL, flags | O_NONBLOCK);

    // Start listening
    if (listen(m_tcpServerSocket, 5) < 0) {
        m_log->error("Failed to listen on TCP socket: {}", strerror(errno));
        ::close(m_tcpServerSocket);
        m_tcpServerSocket = -1;
        return false;
    }

    m_log->info("TCP server socket listening on port {}", m_port);
    return true;
}

bool TcpServerTransport::setupUdpSocket() {
    m_log->debug("Setting up UDP socket for broadcasts");

    m_udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_udpSocket < 0) {
        m_log->error("Failed to create UDP socket: {}", strerror(errno));
        return false;
    }

    // Set socket timeout
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    setsockopt(m_udpSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // Enable SO_REUSEADDR
    int reuse = 1;
    setsockopt(m_udpSocket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // Bind to discovery port
    struct sockaddr_in udp_addr;
    memset(&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    udp_addr.sin_port = htons(DOIP_UDP_DISCOVERY_PORT);

    if (bind(m_udpSocket, reinterpret_cast<struct sockaddr *>(&udp_addr), sizeof(udp_addr)) < 0) {
        m_log->error("Failed to bind UDP socket to port {}: {}", DOIP_UDP_DISCOVERY_PORT, strerror(errno));
        ::close(m_udpSocket);
        m_udpSocket = -1;
        return false;
    }

    m_log->info("UDP socket bound to port {}", DOIP_UDP_DISCOVERY_PORT);
    return true;
}

void TcpServerTransport::configureBroadcast() {
    if (m_loopback) {
        m_log->debug("Configuring for loopback mode");
        m_broadcastAddress.sin_family = AF_INET;
        m_broadcastAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
        m_broadcastAddress.sin_port = htons(DOIP_UDP_TEST_EQUIPMENT_REQUEST_PORT);
    } else {
        m_log->debug("Configuring for broadcast mode");

        // Enable broadcast
        int broadcast = 1;
        if (setsockopt(m_udpSocket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
            m_log->warn("Failed to enable broadcast: {}", strerror(errno));
        }

        m_broadcastAddress.sin_family = AF_INET;
        m_broadcastAddress.sin_addr.s_addr = inet_addr("255.255.255.255");
        m_broadcastAddress.sin_port = htons(DOIP_UDP_TEST_EQUIPMENT_REQUEST_PORT);
    }
}

std::unique_ptr<IConnectionTransport> TcpServerTransport::acceptConnection() {
    if (!m_isActive || m_tcpServerSocket < 0) {
        return nullptr;
    }

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_socket = accept(m_tcpServerSocket, reinterpret_cast<struct sockaddr *>(&client_addr), &client_len);

    if (client_socket < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
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

ssize_t TcpServerTransport::sendBroadcast(const DoIPMessage &msg, uint16_t port) {
    if (m_udpSocket < 0) {
        m_log->error("UDP socket not initialized, cannot send broadcast");
        return -1;
    }

    // Override port if specified
    struct sockaddr_in dest_addr = m_broadcastAddress;
    if (port != 0) {
        dest_addr.sin_port = htons(port);
    }

    ssize_t sent = sendto(
        m_udpSocket,
        msg.data(),
        msg.size(),
        0,
        reinterpret_cast<struct sockaddr *>(&dest_addr),
        sizeof(dest_addr));

    if (sent < 0) {
        m_log->error("Failed to send broadcast: {}", strerror(errno));
        return -1;
    }

    m_log->debug("Sent {} bytes via UDP broadcast", sent);
    return sent;
}

void TcpServerTransport::close() {
    bool expected = true;
    if (m_isActive.compare_exchange_strong(expected, false)) {
        m_log->info("Closing TCP server transport");

        if (m_tcpServerSocket >= 0) {
            ::close(m_tcpServerSocket);
            m_tcpServerSocket = -1;
        }

        if (m_udpSocket >= 0) {
            ::close(m_udpSocket);
            m_udpSocket = -1;
        }
    }
}

bool TcpServerTransport::isActive() const {
    return m_isActive.load();
}

std::string TcpServerTransport::getIdentifier() const {
    return "TCP-Server:0.0.0.0:" + std::to_string(m_port);
}

} // namespace doip
