#include <chrono>
#include <csignal>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <thread>

#include "DoIPAddress.h"
#include "DoIPServer.h"
#include "Logger.h"

#include "DoIPServer.h"
#include "DoIPServerModel.h"
#include "ExampleDoIPServerModel.h"
#include "cli/ServerConfigCLI.h"

#include "util/Daemonize.h"

using namespace doip;

std::unique_ptr<DoIPServer> server;
static std::atomic<bool> g_stopRequested{false};

static void handle_signal(int) {
    std::cerr << "Signal received, stopping server..." << std::endl;
    g_stopRequested.store(true);
    if (server) {
        server->stop();
    }
}

int main(int argc, char *argv[]) {
    ServerConfig cfg;
    cfg.loopback = true;                                            // For testing, use loopback announcements
    cfg.daemonize = argc > 1 && std::string(argv[1]) == "--daemon"; // For testing, run as daemon
    auto console = spdlog::stdout_color_mt("doip-server");


    Logger::setUseSyslog(cfg.daemonize);
    if (cfg.daemonize) {
        if (!doip::daemon::daemonize()) {
            std::cerr << "Failed to daemonize process" << std::endl;
            return 1;
        }
        // Write PID file for integration tests cleanup
        try {
            std::ofstream pidf("/tmp/doip-discover.pid", std::ios::trunc);
            pidf << getpid() << std::endl;
        } catch (...) {
            // best-effort
        }
    }

    // Install simple signal handlers for graceful shutdown
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);


    // Configure logging
    Logger::setLevel(spdlog::level::debug);

    console->info("Starting DoIP Discovery Server");

    server = std::make_unique<DoIPServer>(cfg);

    server->setFurtherActionRequired(DoIPFurtherAction::NoFurtherAction);
    // for discovery check we use relaxed announcement settings
    server->setAnnounceInterval(1000);
    server->setAnnounceNum(10);

    if (!server->setupUdpSocket()) {
        console->critical("Failed to set up UDP socket");
        server->stop(); // Clean up before exiting
        return 1;
    }

    if (!server->setupTcpSocket([]() { return std::make_unique<ExampleDoIPServerModel>(); })) {
        console->critical("Failed to set up TCP socket");
        return 1;
    }

    console->info("DoIP Server is running. Waiting for connections...");

    while (server->isRunning()) {
        if (g_stopRequested.load()) {
            server->stop();
            break;
        }
        sleep(1);
    }
    console->info("DoIP Server Example terminated");
    if (cfg.daemonize) {
        (void)std::remove("/tmp/doip-discover.pid");
    }
    // Cleanly shutdown loggers to avoid sanitizer leak reports
    doip::Logger::shutdown();
    return 0;
}
