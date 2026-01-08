
#ifndef DOIPCLIENT_H
#define DOIPCLIENT_H

#include "DoIPClientModel.h"
#include "DoIPConfig.h"
#include "DoIPMessage.h"
#include "cli/ServerConfig.h"
#include "util/Logger.h"
#include "util/Socket.h"
#include "util/ThreadSafeQueue.h"

#include <arpa/inet.h>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <stdlib.h>
#include <unistd.h>

namespace doip {

/**
 * @brief DoIP client implementation for diagnostic communication
 *
 * This class implements a DoIP (Diagnostics over IP) client according to ISO 13400.
 * It handles both UDP vehicle discovery and TCP diagnostic communication.
 *
 * @note This is legacy code kept for testing purposes only. Not actively maintained.
 *
 * The client supports:
 * - UDP vehicle discovery (announcements and identification requests)
 * - TCP routing activation and diagnostic messaging
 * - Alive check handling
 * - Automatic reconnection
 *
 * @warning Thread-safe for enqueueing messages, but TCP/UDP threads should not be
 *          accessed concurrently with start/stop operations.
 */
class DoIPClient {
    /**
     * @brief Internal state for managing message reception flow
     */
    enum class ReceiveState {
        WaitForAckOrAliveCheck,    ///< Waiting for acknowledgement or alive check from server
        WaitForDiagnosticMessage,  ///< Waiting for diagnostic response message
        Quit,                      ///< Client is shutting down
    };

  public:
    /**
     * @brief Constructs a DoIP client with optional custom model
     *
     * @param model Client model defining behavior (defaults to standard DoIPClientModel)
     */
    explicit DoIPClient(UniqueDoIPClientModelPtr model = std::make_unique<DoIPClientModel>()) : m_model(std::move(model)) { m_receiveBuf.reserve(DOIP_MAXIMUM_MTU); }

    /**
     * @brief Establishes TCP connection to DoIP server and activates routing
     *
     * Creates a TCP socket, connects to the specified server, and performs
     * routing activation handshake. Starts the TCP receiver thread.
     *
     * @param inet_address Server IP address (default: "127.0.0.1")
     * @param port Server TCP port (default: 13400)
     * @return true if connection and routing activation succeeded, false otherwise
     *
     * @note Blocks until routing activation completes or fails
     */
    [[nodiscard]] bool startTcpConnection(const char *inet_address = "127.0.0.1", uint16_t port = DOIP_TCP_DEFAULT_PORT);

    /**
     * @brief Checks if TCP connection is active
     *
     * @return true if TCP thread is running, false otherwise
     */
    [[nodiscard]] bool isTcpRunning() const noexcept { return m_tcpRunning.load(); }

    /**
     * @brief Attempts to reconnect to the DoIP server
     *
     * Closes existing connection and re-establishes TCP connection using
     * previously stored server address and port.
     * TODO: Currently does not retry on failure - not implemented.
     *
     * @return true if reconnection and routing activation succeeded, false otherwise
     */
    [[nodiscard]] bool reconnectServer();

    /**
     * @brief Closes TCP connection and stops TCP receiver thread
     *
     * Waits for TCP thread to terminate gracefully.
     */
    void closeTcpConnection();

    /**
     * @brief Initializes UDP socket for vehicle discovery
     *
     * Creates and configures UDP socket for sending vehicle identification
     * requests.
     * @return true if UDP socket setup succeeded, false otherwise
     */
    [[nodiscard]] bool startUdpConnection();

    /**
     * @brief Checks if UDP connection is active
     *
     * @return true if UDP thread is running, false otherwise
     */
    [[nodiscard]] bool isUdpRunning() const noexcept { return m_udpRunning.load(); }

    /**
     * @brief Starts UDP listener thread for vehicle announcements
     *
     * Listens on port 13401 for vehicle announcement messages from DoIP entities.
     */
    void startAnnouncementListener();

    /**
     * @brief Closes UDP socket and stops UDP listener thread
     */
    void closeUdpConnection();

    /**
     * @brief Receives and processes a single UDP message
     *
     * @todo Make private later
     */
    void receiveUdpMessage();

    /**
     * @brief Sends vehicle identification request via UDP
     *
     * Broadcasts or unicasts a vehicle identification request to discover
     * available DoIP entities.
     *
     * @param inet_address Target IP address (broadcast or unicast)
     * @return Number of bytes sent, or -1 on error
     *
     * @todo Make private later
     */
    ssize_t sendVehicleIdentificationRequest(const char *inet_address);

    /**
     * @brief Waits for and processes vehicle announcement message
     *
     * Blocks until a vehicle announcement is received or timeout occurs.
     * Extracts VIN, EID, GID, and logical address from the announcement.
     *
     * @return true if valid announcement received, false on timeout or error
     *
     * @todo Make private later
     */
    [[nodiscard]]
    bool receiveVehicleAnnouncement();

    /**
     * @brief Enqueues a DoIP message to the server
     *
     * Thread-safe method to queue messages for transmission. The TCP thread
     * will dequeue and send them asynchronously.
     *
     * @param msg DoIP message to send
     */
    void sendMessage(const DoIPMessage &msg);

    /**
     * @brief Enqueues a diagnostic message to the server
     *
     * Wraps the payload in a DoIP diagnostic message format with appropriate
     * source and target addresses, then enqueues for transmission.
     *
     * @param payload Diagnostic data that will be forwarded to the ECU
     */
    void sendDiagnosticMessage(const ByteArray &payload);

    /**
     * @brief Sets the client's source address for diagnostic communication
     *
     * This address identifies the client in routing activation and diagnostic
     * messages.
     *
     * @param address Client's DoIP logical address (typically in tester range)
     */
    void setSourceAddress(const DoIPAddress &address);

    /**
     * @brief Requests the client to quit gracefully
     *
     * Signals both TCP and UDP threads to terminate and updates receive state.
     * Call closeTcpConnection() and closeUdpConnection() after this to ensure
     * threads have stopped.
     */
    void requestQuit() {
        updateReceiveState(ReceiveState::Quit);
        m_tcpRunning.store(false);
        m_udpRunning.store(false);
    }

    /**
     * @brief Gets the properties of the connected DoIP server
     *
     * @return ServerProperties structure containing server details
     */
    const ServerProperties &getServerProperties() const {
        return m_serverProperties;
    }

  protected:
    /**
     * @brief Updates internal receive state with logging
     *
     * Transitions the client's receive state machine and logs the change.
     * Used to coordinate message handling flow.
     *
     * @param newState Target receive state
     */
    void updateReceiveState(ReceiveState newState) {
        if (m_receiveState != newState) {
            m_log->info("Receive state changed from {} to {}", static_cast<int>(m_receiveState), static_cast<int>(newState));
            m_receiveState = newState;
        }
    }

  private:
    UniqueDoIPClientModelPtr m_model;        ///< Client behavior model
    ByteArray m_receiveBuf;                  ///< Buffer for receiving messages
    Socket m_tcpSocket;   ///< TCP connection sockets
    Socket m_udpSocket, m_udpVehicleAnnSocket; ///< UDP discovery sockets
    ThreadSafeQueue<DoIPMessage> m_messageQueue; ///< Outgoing message queue
    std::atomic<bool> m_tcpRunning{false};   ///< TCP thread running flag
    std::atomic<bool> m_udpRunning{false};   ///< UDP thread running flag
    std::thread m_tcpThread{};               ///< TCP receiver thread
    int m_broadcast = 1;                     ///< Broadcast socket option flag
    struct sockaddr_in m_serverAddress, m_clientAddress, m_announcementAddress; ///< Socket addresses

    std::shared_ptr<spdlog::logger> m_log = spdlog::stdout_color_mt("doip-client");

    DoIPAddress m_sourceAddress = DoIPAddress(0xE000); ///< Client's logical address
    ServerProperties m_serverProperties{};   ///< Properties of the connected server
    DoIPFurtherAction m_furtherActionReqResult = DoIPFurtherAction::NoFurtherAction; ///< Further action required flag

    ReceiveState m_receiveState = ReceiveState::WaitForAckOrAliveCheck; ///< Current receive state

    /**
     * @brief Sends routing activation request to server
     *
     * @return Number of bytes sent, or -1 on error
     */
    ssize_t sendRoutingActivationRequest();

    /**
     * @brief Receives and validates routing activation response
     *
     * @return Routing activation response message if received, std::nullopt otherwise
     */
    std::optional<DoIPMessage> receiveRoutingActivationResponse();

    /**
     * @brief Sends alive check response containing the client's source address to the server
     *
     * Responds to server's alive check request to maintain the connection.
     *
     * @return Number of bytes sent, or -1 on error
     */
    ssize_t sendAliveCheckResponse();

    /**
     * @brief Main TCP receiver thread function
     *
     * Continuously receives messages from server and dispatches them for processing.
     * Also dequeues and sends messages from the outgoing queue.
     */
    void tcpThreadFunction();

    /**
     * @brief Performs routing activation handshake with server
     *
     * Sends routing activation request and waits for positive response.
     *
     * @return true if routing successfully activated, false otherwise
     */
    [[nodiscard]] bool activateRouting();

    /**
     * @brief Sends a DoIP message over TCP connection
     *
     * @param msg Message to send
     * @return Number of bytes sent, or -1 on error
     */
    [[nodiscard]] ssize_t sendDoIPMessage(const DoIPMessage &msg);

    /**
     * @brief Receives a DoIP message from TCP connection
     *
     * Reads and parses a complete DoIP message from the socket.
     *
     * @return Received message if successful, std::nullopt on error or connection closed
     */
    [[nodiscard]] std::optional<DoIPMessage> receiveMessage();

    /**
     * @brief Processes received DoIP message based on type
     *
     * Dispatches message to appropriate handler based on payload type.
     * Handles acknowledgements, alive checks, and diagnostic responses.
     *
     * @param msg Received message to process
     */
    void handleMessage(const DoIPMessage &msg);

    /**
     * @brief Main UDP listener thread function
     *
     * Continuously listens for vehicle announcements on UDP port 13401.
     */
    void udpThreadFunction();

    /**
     * @brief Parses vehicle identification response message
     *
     * Extracts VIN, logical address, EID, GID, and further action requirements
     * from the vehicle announcement message.
     *
     * @param msg Vehicle identification response message
     */
    void parseVehicleIdentificationResponse(const DoIPMessage &msg);

    int emptyMessageCounter = 0; ///< Counter for consecutive empty message receives

    std::mutex m_mutex; ///< Mutex for protecting connection operations
};

} // namespace doip

#endif /* DOIPCLIENT_H */
