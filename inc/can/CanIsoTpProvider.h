#ifndef CANISOTPPROVIDER_H
#define CANISOTPPROVIDER_H

#include <cstring>
#include <linux/can.h>
#include <linux/can/isotp.h>
#include <net/if.h>
#include <stdexcept>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "IDownstreamProvider.h"
#include "Logger.h"
#include "Socket.h"

namespace doip::can {

using namespace doip;

/**
 * @brief  Configuration structure for ISO-TP over SocketCAN.
 * Used to set various ISO-TP options on the socket.
 */
struct IsoTpConfig {
    std::optional<uint8_t> fc_st_min;
    std::optional<uint8_t> fc_bs;

    bool hasFlowControlOption() const {
        return fc_st_min.has_value() || fc_bs.has_value();
    }
};

/**
 * @brief SocketCAN-based downstream provider using ISO-TP.
 *
 * @note: CAN interface must be configured and brought up at system level:
 * ```bash
 * sudo ip link set <interface> type can bitrate <bitrate>
 * sudo ip link set <interface> up
 * ```
 */
class CanIsoTpProvider : public IDownstreamProvider {
  public:
    explicit CanIsoTpProvider(const std::string &interfaceName, uint32_t tx_address, uint32_t rx_address, std::optional<IsoTpConfig> options = std::nullopt)
        : m_interfaceName(interfaceName), m_txAddress(tx_address), m_rxAddress(rx_address), m_options(options), m_logger(Logger::get("can-isotp")){

                                                                                                                };

    virtual ~CanIsoTpProvider() noexcept {
        m_canSocket.close();
        m_logger->info("CAN ISO-TP provider stopped");
    }

    virtual void start() override {
        if (m_canSocket) {
            return; // Already started
        }

        int tmpSocket = -1;

        m_logger->info("Starting CAN ISO-TP on interface '{}' (TX ID: 0x{:X}, RX ID: 0x{:X})...",
                       m_interfaceName, m_txAddress, m_rxAddress);
        // Create CAN ISO-TP socket
        tmpSocket = socket(PF_CAN, SOCK_DGRAM, CAN_ISOTP);
        if (tmpSocket < 0) {
            throw std::runtime_error("Failed to create CAN ISO-TP socket: " + std::string(strerror(errno)));
        }

        // Get interface index
        struct ifreq ifr;
        std::strncpy(ifr.ifr_name, m_interfaceName.c_str(), IFNAMSIZ - 1);
        ifr.ifr_name[IFNAMSIZ - 1] = '\0';

        if (ioctl(tmpSocket, SIOCGIFINDEX, &ifr) < 0) {
            close(tmpSocket);
            throw std::runtime_error("Failed to get interface index for " + m_interfaceName + ": " + std::string(strerror(errno)));
        }

        // Configure ISO-TP options
        struct can_isotp_options opts;
        std::memset(&opts, 0, sizeof(opts));
        opts.flags = CAN_ISOTP_TX_PADDING | CAN_ISOTP_RX_PADDING;

        if (setsockopt(tmpSocket, SOL_CAN_ISOTP, CAN_ISOTP_OPTS, &opts, sizeof(opts)) < 0) {
            close(tmpSocket);
            throw std::runtime_error("Failed to set ISO-TP options: " + std::string(strerror(errno)));
        }

        // Configure ISO-TP flow control options
        if (m_options && m_options.value().hasFlowControlOption()) {
            IsoTpConfig cfg = m_options.value();
            struct can_isotp_fc_options fcopts;
            std::memset(&fcopts, 0, sizeof(fcopts));
            fcopts.bs = cfg.fc_bs.value_or(0);        // Block size: 0 = no flow control
            fcopts.stmin = cfg.fc_st_min.value_or(0); // Separation time minimum (0-127ms or 100-900us)
            fcopts.wftmax = 0;                        // Wait frame max transmissions

            if (setsockopt(tmpSocket, SOL_CAN_ISOTP, CAN_ISOTP_RECV_FC, &fcopts, sizeof(fcopts)) < 0) {
                close(tmpSocket);
                throw std::runtime_error("Failed to set ISO-TP flow control options: " + std::string(strerror(errno)));
            }
        }

        // Bind socket to CAN interface with TX tx_address
        struct sockaddr_can addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.can_family = AF_CAN;
        addr.can_ifindex = ifr.ifr_ifindex;
        addr.can_addr.tp.tx_id = m_txAddress; // Physical TX address
        addr.can_addr.tp.rx_id = m_rxAddress; // Physical RX address

        if (bind(tmpSocket, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0) {
            close(tmpSocket);
            throw std::runtime_error("Failed to bind CAN socket: " + std::string(strerror(errno)));
        }

        m_logger->info("CAN ISO-TP socket successfully bound to interface '{}' (index {})", m_interfaceName, ifr.ifr_ifindex);
        // ok -> transfer ownership
        m_canSocket.reset(tmpSocket);
    }

    virtual void stop() override {
        m_canSocket.close();
    }

    virtual void sendRequest(const ByteArray request, DownstreamCallback cb) override {
        if (!cb)
            return;

        auto start_ts = std::chrono::steady_clock::now();

        // send request
        auto sentBytes = send(m_canSocket.get(), request.data(), request.size(), 0);
        if (sentBytes < 0) {
            m_logger->error("Timeout or error sending CAN ISO-TP request: {}", strerror(errno));
            DownstreamResponse dr;
            dr.status = DownstreamStatus::Error;
            cb(dr);
            return;
        }

        // receive response

        std::array<uint8_t, 4096> rspBuffer{};

        ssize_t receivedBytes = 0;
        m_logger->info("Waiting for CAN ISO-TP response...");
        do {
            receivedBytes = recv(m_canSocket.get(), rspBuffer.data(), rspBuffer.size(), 0);
            m_logger->info("Receive returned {} (bytes)...", receivedBytes);
            if (receivedBytes < 0) {
                DownstreamResponse dr;
                dr.status = DownstreamStatus::Timeout;
                cb(dr);
                m_logger->error("Timeout or error receiving CAN ISO-TP response: {}", strerror(errno));
                return;
            } else if (receivedBytes == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        } while (receivedBytes == 0);

        m_logger->info("Received {} bytes from CAN ISO-TP", receivedBytes);
        ByteArray rsp(rspBuffer.data(), static_cast<size_t>(receivedBytes)); // ISO-TP max is 4095 bytes
        m_logger->info("Response {} ", fmt::streamed(rsp));
        rsp.resize(static_cast<size_t>(receivedBytes));
        m_logger->info("Response after resize {} ", fmt::streamed(rsp));

        auto end_ts = std::chrono::steady_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end_ts - start_ts);

        DownstreamResponse dr;
        dr.payload = rsp;
        dr.latency = latency;
        dr.status = DownstreamStatus::Handled;
        cb(dr);
    }

    virtual std::string_view getProviderName() const override {
        return "CanIsoTpProvider";
    }

  private:
    std::string m_interfaceName;
    uint32_t m_txAddress;
    uint32_t m_rxAddress;
    std::optional<IsoTpConfig> m_options;
    Socket m_canSocket;
    std::shared_ptr<spdlog::logger> m_logger;
};

} // namespace doip::can

#endif /* CANISOTPPROVIDER_H */
