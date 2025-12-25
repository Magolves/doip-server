#include "DoIPServer.h"
#include "DoIPConnection.h"
#include "DoIPMessage.h"
#include "DoIPServerModel.h"
#include "MacAddress.h"
#include "TcpServerTransport.h"
#include <algorithm> // for std::remove_if
#include <cerrno>    // for errno
#include <cstdio>
#include <cstring> // for strerror
#include <fcntl.h>
#include <fstream> // for PID file writing
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

using namespace doip;

DoIPServer::~DoIPServer() noexcept {
    if (m_udpRunning.load() || m_tcpRunning.load()) {
        stop();
    }
}

DoIPServer::DoIPServer(const ServerConfig &config)
    : m_config(config),
      m_doipLog(Logger::get("server")),
      m_udpLog(Logger::getUdp()),
      m_tcpLog(Logger::getTcp()),
      m_transport(std::make_unique<TcpServerTransport>(config.loopback)) {

    setLoopbackMode(m_config.loopback);
}

/*
 * Stop the server and cleanup
 */
void DoIPServer::stop() {
    m_doipLog->info("Stopping DoIP server...");
    m_udpRunning.store(false);
    m_tcpRunning.store(false);

    // Wait for all threads to finish BEFORE closing transport
    for (auto &thread : m_workerThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    m_workerThreads.clear();

    m_doipLog->info("Server stopped, closing transport...");
    if (m_transport) {
        m_transport->close();
    }
}

/*
 * Background thread: Handle individual TCP connection
 */
void DoIPServer::connectionHandlerThread(std::unique_ptr<DoIPDefaultConnection> connection) {
    m_tcpLog->info("Connection handler thread started");
    auto closeReason = DoIPCloseReason::ApplicationRequest;

    while (m_tcpRunning.load()) {
        auto msg = connection->receiveProtocolMessage();

        if (!msg.has_value()) {
            m_tcpLog->info("Connection closed or error occurred");
            closeReason = DoIPCloseReason::SocketError;
            break;
        } else {
            connection->handleMessage2(msg.value());
        }
    }
    connection->closeConnection(closeReason);
    // Connection is automatically closed when unique_ptr goes out of scope
    m_tcpLog->info("Connection to {} thread exit {}", connection->getClientAddress(), fmt::streamed(closeReason));
}

/*
 * Set up a tcp socket, so the socket is ready to accept a connection
 */
bool DoIPServer::setupTcpSocket(std::function<UniqueServerModelPtr()> modelFactory) {
    m_doipLog->debug("Setting up TCP transport on port {}", DOIP_SERVER_TCP_PORT);

    if (!m_transport) {
        m_doipLog->error("Transport not initialized");
        return false;
    }

    if (!m_transport->setup(DOIP_SERVER_TCP_PORT)) {
        m_doipLog->error("Failed to setup transport");
        return false;
    }

    // Success - start TCP acceptor thread
    m_modelFactory = modelFactory;
    m_tcpRunning.store(true);
    m_workerThreads.emplace_back([this, factory = std::move(modelFactory)]() mutable {
        tcpListenerThread(std::move(factory));
    });

    m_tcpLog->info("TCP transport ready and listening on port {}", DOIP_SERVER_TCP_PORT);
    return true;
}

/*
 * Closes the TCP transport
 */
void DoIPServer::closeTcpSocket() {
    if (m_transport) {
        m_transport->close();
    }
}

bool DoIPServer::setupUdpSocket() {
    // UDP is already setup in TcpServerTransport
    m_udpRunning.store(true);
    m_workerThreads.emplace_back([this]() {
        udpAnnouncementThread();
    });

    return true;
}

void DoIPServer::closeUdpSocket() {
    m_udpRunning.store(false);
    // Transport handles UDP socket cleanup
}


bool DoIPServer::setDefaultEid() {
    MacAddress mac = {0};
    if (!getFirstMacAddress(mac)) {
        m_doipLog->error("Failed to get MAC address, using default EID");
        m_config.eid = DoIpEid::Zero;
        return false;
    }
    // Set EID based on MAC address (last 6 bytes)
    m_config.eid = DoIpEid(mac.data(), m_config.eid.ID_LENGTH);
    return true;
}

void DoIPServer::setVin(const std::string &VINString) {

    m_config.vin = DoIpVin(VINString);
}

void DoIPServer::setVin(const DoIpVin &vin) {
    if (!isValidVin(vin)) {
        m_doipLog->warn("Invalid VIN provided {}", fmt::streamed(vin));
    }
    m_config.vin = vin;
}

void DoIPServer::setLogicalGatewayAddress(DoIPAddress logicalAddress) {
    m_config.logicalAddress = logicalAddress;
}

void DoIPServer::setEid(const uint64_t inputEID) {
    m_config.eid = DoIpEid(inputEID);
}

void DoIPServer::setGid(const uint64_t inputGID) {
    m_config.gid = DoIpGid(inputGID);
}

void DoIPServer::setFurtherActionRequired(DoIPFurtherAction furtherActionRequired) {
    m_FurtherActionReq = furtherActionRequired;
}

void DoIPServer::setAnnounceNum(int Num) {
    m_config.announceCount = Num;
}

void DoIPServer::setAnnounceInterval(unsigned int Interval) {
    m_config.announceInterval = Interval;
}

void DoIPServer::setLoopbackMode(bool useLoopback) {
    m_config.loopback = useLoopback;
    if (m_config.loopback) {
        m_doipLog->info("Vehicle announcements will use loopback (127.0.0.1)");
    } else {
        m_doipLog->info("Vehicle announcements will use broadcast (255.255.255.255)");
    }
}



// new version starts here

void DoIPServer::udpAnnouncementThread() {
    m_doipLog->info("Announcement thread started");

    if (!m_transport) {
        m_doipLog->error("No transport available for announcements");
        return;
    }

    // Send announcements with configured interval and count
    for (int i = 0; i < m_config.announceCount && m_udpRunning; i++) {
        DoIPMessage msg = message::makeVehicleIdentificationResponse(
            m_config.vin, m_config.logicalAddress, m_config.eid, m_config.gid);

        ssize_t sentBytes = m_transport->sendBroadcast(msg, DOIP_UDP_TEST_EQUIPMENT_REQUEST_PORT);

        m_doipLog->info("TX {}", fmt::streamed(msg));
        if (sentBytes > 0) {
            m_udpLog->info("Sent Vehicle Announcement: {} bytes", sentBytes);
        } else {
            m_udpLog->error("Failed to send announcement");
        }

        usleep(m_config.announceInterval * 1000);
    }

    m_doipLog->info("Announcement thread stopped");
}

std::unique_ptr<DoIPConnection> DoIPServer::waitForTcpConnection(std::function<UniqueServerModelPtr()> modelFactory) {
    if (!m_tcpRunning.load() || !m_transport) {
        return nullptr;
    }

    // Use transport layer to accept connection
    auto connectionTransport = m_transport->acceptConnection();
    if (!connectionTransport) {
        return nullptr; // No connection available (timeout or error)
    }

    auto model = modelFactory ? modelFactory() : std::make_unique<DefaultDoIPServerModel>();
    m_tcpLog->info("Accepted new TCP connection, model {} (factory {})",
                   model->getModelName(),
                   modelFactory ? "provided" : "default");

    return std::unique_ptr<DoIPConnection>(
        new DoIPConnection(std::move(connectionTransport), std::move(model), m_TimerManager));
}

void DoIPServer::tcpListenerThread(std::function<UniqueServerModelPtr()> modelFactory) {

    m_doipLog->info("TCP listener thread started");

    while (m_tcpRunning.load()) {

        auto connection = waitForTcpConnection(modelFactory);

        if (!connection) {
            if (m_tcpRunning.load()) {
                m_tcpLog->debug("Failed to accept connection, retrying...");
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            continue;
        }

        // Spawn a dedicated thread for this connection
        // -- Note: We detach because the connection thread manages its own lifecycle
        m_workerThreads.emplace_back(std::thread(&DoIPServer::connectionHandlerThread, this, std::move(connection)));
    }

    m_doipLog->info("TCP listener thread stopped");
}
