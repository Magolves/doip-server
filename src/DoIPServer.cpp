#include <algorithm> // for std::remove_if
#include <cerrno>    // for errno
#include <cstring>   // for strerror
#include <fcntl.h>
#include <fstream>   // for PID file writing
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "DoIPConnection.h"
#include "DoIPMessage.h"
#include "DoIPServer.h"
#include "DoIPServerModel.h"
#include "MacAddress.h"

using namespace doip;

DoIPServer::~DoIPServer() {
    if (m_udpRunning.load() || m_tcpRunning.load()) {
        stop();
    }
}

DoIPServer::DoIPServer(const ServerConfig &config)
    : m_config(config) {

    // Daemonize FIRST if requested, before any logging or resource initialization
    if (m_config.daemonize) {
        daemonize();
    }

    // Initialize loggers AFTER daemonization to avoid fork() issues with spdlog
    m_doipLog = Logger::get("doip");
    m_udpLog = Logger::get("udp ");
    m_tcpLog = Logger::get("tcp ");

    setLoopbackMode(m_config.loopback);
}

void DoIPServer::daemonize() {
    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "First fork failed: " << strerror(errno) << std::endl;
        return;
    }

    if (pid > 0) {
        // Parent process: print child PID and exit
        std::cout << "Started daemon with PID " << pid << '\n';
        _exit(0);
    }

    // Child: create new session and become session leader
    if (setsid() < 0) {
        std::cerr << "setsid failed: " << strerror(errno) << std::endl;
        return;
    }

    // Second fork to ensure the daemon can't reacquire a tty
    pid = fork();
    if (pid < 0) {
        std::cerr << "Second fork failed: " << strerror(errno) << std::endl;
        return;
    }
    if (pid > 0) {
        _exit(0);
    }

    // Final daemon process: get our PID and write to file for test integration
    pid_t daemon_pid = getpid();

    // Write PID to file for testing/monitoring
    std::ofstream pid_file("/tmp/doip_server.pid");
    if (pid_file.is_open()) {
        pid_file << daemon_pid << std::endl;
        pid_file.close();
    }

    // Set file mode creation mask to a safe default
    umask(0);

    // DON'T change working directory - it breaks relative paths for sockets/files
    // Production daemons should chdir("/"), but for testing we need access to local resources

    // Close and redirect standard file descriptors to /dev/null
    int fd = open("/dev/null", O_RDWR);
    if (fd >= 0) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > STDERR_FILENO) {
            close(fd);
        }
    }
    // Note: can't log here yet as loggers aren't initialized
}

/*
 * Stop the server and cleanup
 */
void DoIPServer::stop() {
    m_udpRunning.store(false);
    m_tcpRunning.store(false);

    // Wait for all threads to finish BEFORE closing sockets
    for (auto &thread : m_workerThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    m_workerThreads.clear();

    closeUdpSocket();
    closeTcpSocket();
}

/*
 * Background thread: Handle individual TCP connection
 */
void DoIPServer::connectionHandlerThread(std::unique_ptr<DoIPConnection> connection) {
    m_tcpLog->info("Connection handler thread started");

    while (m_udpRunning.load() && connection->isSocketActive()) {
        int result = connection->receiveTcpMessage();

        if (result < 0) {
            m_tcpLog->info("Connection closed or error occurred");
            break;
        }
    }

    // Connection is automatically closed when unique_ptr goes out of scope
    m_tcpLog->info("Connection handler thread stopped");
}

/*
 * Set up a tcp socket, so the socket is ready to accept a connection
 * TODO: Store model factory for later use when accepting connections
 */
bool DoIPServer::setupTcpSocket(std::function<UniqueServerModelPtr()> modelFactory) {
    m_doipLog->debug("Setting up TCP socket on port {}", DOIP_SERVER_TCP_PORT);

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        m_tcpLog->error("Failed to create TCP socket: {}", strerror(errno));
        return false;
    }

    // Allow socket reuse
    int reuse = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        m_tcpLog->warn("Failed to set SO_REUSEADDR: {}", strerror(errno));
    }

    m_serverAddress.sin_family = AF_INET;
    m_serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    m_serverAddress.sin_port = htons(DOIP_SERVER_TCP_PORT);

    // binds the socket to the address and port number
    if (bind(sock_fd, reinterpret_cast<const struct sockaddr *>(&m_serverAddress), sizeof(m_serverAddress)) < 0) {
        m_tcpLog->error("Failed to bind TCP socket: {}", strerror(errno));
        ::close(sock_fd);
        return false;
    }

    // Put the socket into listening state so the OS reports LISTEN and accept() can proceed
    if (listen(sock_fd, 5) < 0) {
        m_tcpLog->error("Failed to listen on TCP socket: {}", strerror(errno));
        ::close(sock_fd);
        return false;
    }

    // Success - transfer ownership to Socket wrapper
    m_tcpSock.reset(sock_fd);
    m_modelFactory = modelFactory;

    // Also start TCP acceptor thread so TCP 13400 enters LISTEN state and accepts connections
    m_tcpRunning.store(true);
    m_workerThreads.emplace_back([this, modelFactory]() { tcpListenerThread(modelFactory); });

    m_tcpLog->info("TCP socket bound and listening on port {}", DOIP_SERVER_TCP_PORT);
    return true;
}

/*
 * Closes the socket for this server
 */
void DoIPServer::closeTcpSocket() {
    m_tcpSock.close();
}

bool DoIPServer::setupUdpSocket() {
    m_udpLog->debug("Setting up UDP socket on port {}", DOIP_UDP_DISCOVERY_PORT);

    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        perror("Failed to create socket");
        return false;
    }

    // Set socket to non-blocking with timeout
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // Enable SO_REUSEADDR
    int reuse = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // Bind socket to port 13400
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(DOIP_UDP_DISCOVERY_PORT);

    if (bind(sock_fd, reinterpret_cast<struct sockaddr *>(&server_addr), sizeof(server_addr)) < 0) {
        perror("Failed to bind socket");
        ::close(sock_fd);
        return false;
    }

    // Success - transfer ownership to Socket wrapper
    m_udpLock.reset(sock_fd);

    // setting the IP DoIPAddress for Multicast/Broadcast
    if (!m_config.loopback) { //
        setMulticastGroup("224.0.0.2");
        m_udpLog->info("UDP socket successfully bound to port {} with multicast group", DOIP_UDP_DISCOVERY_PORT);
    } else {
        m_udpLog->info("UDP socket successfully bound to port {} with broadcast", DOIP_UDP_DISCOVERY_PORT);
    }

    m_udpLog->debug(
        "Socket fd={} bound to {}:{}",
        m_udpLock.get(),
        inet_ntoa(server_addr.sin_addr),
        ntohs(server_addr.sin_port));

    m_udpRunning.store(true);
    m_workerThreads.emplace_back([this]() { udpListenerThread(); });
    m_workerThreads.emplace_back([this]() { udpAnnouncementThread(); });

    return true;
}

void DoIPServer::closeUdpSocket() {
    m_udpRunning.store(false);
    for (auto &thread : m_workerThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    m_udpLock.close();
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

void DoIPServer::setMulticastGroup(const char *address) const {
    int loop = 1;

    // set Option using the same Port for multiple Sockets
    int setPort = setsockopt(m_udpLock.get(), SOL_SOCKET, SO_REUSEADDR, &loop, sizeof(loop));

    if (setPort < 0) {
        m_udpLog->error("Setting Port Error");
    }

    struct ip_mreq mreq;

    mreq.imr_multiaddr.s_addr = inet_addr(address);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    // set Option to join Multicast Group
    int setGroup = setsockopt(m_udpLock.get(), IPPROTO_IP, IP_ADD_MEMBERSHIP, reinterpret_cast<const char *>(&mreq), sizeof(mreq));

    if (setGroup < 0) {
        m_udpLog->error("Setting address failed: {}", strerror(errno));
    }
}

ssize_t DoIPServer::sendNegativeUdpAck(DoIPNegativeAck ackCode) {
    DoIPMessage msg = message::makeNegativeAckMessage(ackCode);

    return sendUdpResponse(msg);
}

// new version starts here
void DoIPServer::udpListenerThread() {
    socklen_t client_len = sizeof(m_clientAddress);

    m_udpLog->info("UDP listener thread started");

    while (m_udpRunning) {
        ssize_t received = recvfrom(m_udpLock.get(), m_receiveBuf.data(), sizeof(m_receiveBuf), 0,
                                    reinterpret_cast<struct sockaddr *>(&m_clientAddress), &client_len);

        if (received < 0) {
            if (errno == EAGAIN /* || errno == EWOULDBLOCK*/) {
                // Timeout, continue
                continue;
            }
            if (m_udpRunning) {
                perror("recvfrom error");
            }
            break;
        }

        if (received > 0) {
            std::scoped_lock lock(m_mutex);
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &m_clientAddress.sin_addr, client_ip, sizeof(client_ip));
            m_clientIp = std::string(client_ip);
            m_clientPort = ntohs(m_clientAddress.sin_port);

            m_udpLog->info("Received {} bytes from {}:{}", received, m_clientIp, m_clientPort);

            auto optHeader = DoIPMessage::tryParseHeader(m_receiveBuf.data(), DOIP_HEADER_SIZE);
            if (!optHeader.has_value()) {
                auto sentBytes = sendNegativeUdpAck(DoIPNegativeAck::IncorrectPatternFormat);
                if (sentBytes < 0) {
                    if (errno == EAGAIN /*|| errno == EWOULDBLOCK*/) {
                        usleep(100);
                        continue;
                    }
                    break;
                }
            }
            auto plType = optHeader->first;
            // auto payloadLength = optHeader->second;
            m_udpLog->info("RX: {}", fmt::streamed(plType));

            ssize_t sentBytes = 0;
            switch (plType) {
            case DoIPPayloadType::VehicleIdentificationRequest: {
                DoIPMessage msg = message::makeVehicleIdentificationResponse(m_config.vin, m_config.logicalAddress, m_config.eid, m_config.gid);
                sentBytes = sendUdpResponse(msg);
            } break;

            default:
                m_doipLog->error("Invalid payload type 0x{:04X} received (receiveUdpMessage())", static_cast<uint16_t>(plType));
                sentBytes = sendNegativeUdpAck(DoIPNegativeAck::UnknownPayloadType);

            } // switch

            if (sentBytes < 0) {
                if (errno == EAGAIN /*|| errno == EWOULDBLOCK*/) {
                    usleep(100);
                    continue;
                }
                break;
            }
        }
    }

    m_udpLog->info("UDP listener thread stopped");
}

void DoIPServer::udpAnnouncementThread() {
    m_doipLog->info("Announcement thread started");

    // Send announcements with configured interval and count
    for (int i = 0; i < m_config.announceCount && m_udpRunning; i++) {
        sendVehicleAnnouncement();
        usleep(m_config.announceInterval * 1000);
    }

    m_doipLog->info("Announcement thread stopped");
}

ssize_t DoIPServer::sendVehicleAnnouncement() {
    DoIPMessage msg = message::makeVehicleIdentificationResponse(m_config.vin, m_config.logicalAddress, m_config.eid, m_config.gid);

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(DOIP_UDP_TEST_EQUIPMENT_REQUEST_PORT);

    const char *dest_ip;
    if (m_config.loopback) {
        dest_ip = "127.0.0.1";
        inet_pton(AF_INET, dest_ip, &dest_addr.sin_addr);
    } else {
        dest_ip = "255.255.255.255";
        dest_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

        // Enable broadcast
        int broadcast = 1;
        setsockopt(m_udpLock.get(), SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    }

    ssize_t sentBytes = sendto(m_udpLock.get(), msg.data(), msg.size(), 0,
                               reinterpret_cast<struct sockaddr *>(&dest_addr), sizeof(dest_addr));

    m_doipLog->info("TX {}", fmt::streamed(msg));
    if (sentBytes > 0) {
        m_udpLog->info("Sent Vehicle Announcement: {} bytes to {}:{}",
                     sentBytes, dest_ip, DOIP_UDP_TEST_EQUIPMENT_REQUEST_PORT);
    } else {
        m_udpLog->error("Failed to send announcement: {}", strerror(errno));
    }
    return sentBytes;
}

ssize_t DoIPServer::sendUdpResponse(DoIPMessage msg) {
    auto sentBytes = sendto(m_udpLock.get(), msg.data(), msg.size(), 0,
                            reinterpret_cast<struct sockaddr *>(&m_clientAddress), sizeof(m_clientAddress));

    if (sentBytes > 0) {
        m_doipLog->info("TX {}", fmt::streamed(msg));
        m_udpLog->info("Sent UDS response: {} bytes to {}:{}",
                     sentBytes, m_clientIp, ntohs(m_clientAddress.sin_port));
    } else {
        m_doipLog->error("Failed to send message: {}", strerror(errno));
    }
    return sentBytes;
}

std::unique_ptr<DoIPConnection> DoIPServer::waitForTcpConnection(UniqueServerModelPtr model) {
    // waits till client approach to make connection
    if (listen(m_tcpSock.get(), 5) < 0) {
        return nullptr;
    }

    int tcpSocket = accept(m_tcpSock.get(), nullptr, nullptr);
    if (tcpSocket < 0) {
        return nullptr;
    }

    return std::unique_ptr<DoIPConnection>(new DoIPConnection(tcpSocket, std::move(model), m_TimerManager));
}

void DoIPServer::tcpListenerThread(std::function<UniqueServerModelPtr()> modelFactory) {
    m_doipLog->info("TCP listener thread started");

    while (m_tcpRunning.load()) {
        auto model = modelFactory ? modelFactory() : std::make_unique<DefaultDoIPServerModel>();
        auto connection = waitForTcpConnection(std::move(model));

        if (!connection) {
            if (m_tcpRunning.load()) {
                m_tcpLog->debug("Failed to accept connection, retrying...");
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            continue;
        }

        // Spawn a dedicated thread for this connection
        // Note: We detach because the connection thread manages its own lifecycle
        std::thread(&DoIPServer::connectionHandlerThread, this, std::move(connection)).detach();
    }

    m_doipLog->info("TCP listener thread stopped");
}
