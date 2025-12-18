#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>

#include "DoIPAddress.h"
#include "DoIPServer.h"
#include "Logger.h"

#include "CanIsoTpServerModel.h"
#include "DoIPServer.h"
#include "can/CanIsoTpProvider.h"
#include "cli/ServerConfigCLI.h"

using namespace doip;
using namespace std;

namespace {
std::unique_ptr<DoIPServer> server;
std::vector<std::thread> doipReceiver;
std::unique_ptr<DoIPConnection> tcpConnection(nullptr);

const std::string &interfaceName{"vcan0"};
uint32_t tx_address = 0x98DA28F2;  // DoIP server sends to ECU (tester -> ECU)
uint32_t rx_address = 0x98DAF228;  // DoIP server receives from ECU (ECU -> tester)
} // namespace

void listenTcp();

/*
 * Check permanently if tcp message was received
 */
void listenTcp() {
    LOG_UDP_INFO("TCP listener thread started");

    while (true) {
        tcpConnection = server->waitForTcpConnection(std::make_unique<CanIsoTpServerModel>(interfaceName, tx_address, rx_address));

        while (tcpConnection->isSocketActive()) {
            tcpConnection->receiveTcpMessage();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

int main(int argc, char *argv[]) {
    // Parse command line arguments using ServerConfigCLI
    doip::ServerConfig cfg;
    cli::ServerConfigCLI cli;
    try {
        cfg = cli.parse_and_build(argc, argv);
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    // Configure logging
    doip::Logger::setLevel(spdlog::level::debug);
    LOG_DOIP_INFO("Starting DoIP Server Example");

    server = std::make_unique<DoIPServer>(cfg);
    // Apply defaults used previously in example
    if (!server->setupTcpSocket([]() { return std::make_unique<CanIsoTpServerModel>(interfaceName, tx_address, rx_address); })) {
        LOG_DOIP_CRITICAL("Failed to set up TCP socket");
        return 1;
    }

    doipReceiver.push_back(thread(&listenTcp));
    LOG_DOIP_INFO("Starting TCP listener threads");

    while (server->isRunning()) {
        sleep(1);
    }

    doipReceiver.at(0).join();
    LOG_DOIP_INFO("DoIP CAN ISOTP Server Example terminated");
    return 0;
}
