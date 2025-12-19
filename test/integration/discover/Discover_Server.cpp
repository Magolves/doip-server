#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>

#include "DoIPAddress.h"
#include "DoIPServer.h"
#include "Logger.h"

#include "DoIPServer.h"
#include "ExampleDoIPServerModel.h"
#include "DoIPServerModel.h"
#include "cli/ServerConfigCLI.h"

using namespace doip;

std::unique_ptr<DoIPServer> server;


int main() {
    ServerConfig cfg;
    LOG_DOIP_WARN("Loopback/daemon mode forced for testing purposes");
    cfg.loopback = true; // For testing, use loopback announcements
    cfg.daemonize = true; // For testing, run as daemon


    // Configure logging
    Logger::setLevel(spdlog::level::debug);

    LOG_DOIP_INFO("Starting DoIP Discovery Server");

    server = std::make_unique<DoIPServer>(cfg);

    server->setFurtherActionRequired(DoIPFurtherAction::NoFurtherAction);
    // for discovery check we use relaxed announcement settings
    server->setAnnounceInterval(1000);
    server->setAnnounceNum(10);

    if (!server->setupUdpSocket()) {
        LOG_DOIP_CRITICAL("Failed to set up UDP socket");
        server.reset();  // Clean up before exiting
        return 1;
    }

    if (!server->setupTcpSocket([](){ return std::make_unique<ExampleDoIPServerModel>(); })) {
        LOG_DOIP_CRITICAL("Failed to set up TCP socket");
            return 1;
    }

    LOG_DOIP_INFO("DoIP Server is running. Waiting for connections...");

    while(server->isRunning()) {
        sleep(1);
    }
    LOG_DOIP_INFO("DoIP Server Example terminated");
    return 0;
}
