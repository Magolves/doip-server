#include "DoIPServer.h"
#include "uds/UdsServerModel.h"

using namespace doip;
using doip::uds::UdsServerModel;

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    ServerConfig cfg;
    cfg.loopback = true; // For testing, use loopback announcements

    auto server = std::make_unique<DoIPServer>(cfg);
    auto console = spdlog::stdout_color_mt("doip-udp-server");

    // for discovery check we use relaxed announcement settings
    server->setAnnounceInterval(500);  // Send announcements every 500ms for faster discovery
    server->setAnnounceNum(10);       // Send 100 announcements = 50 seconds of announcements (enough for parallel test execution)
    server->setVin("WVWZZZ1JZ3W386752");
    server->setGid(123456);
    server->setEid(654321);

    // Start announcement thread after sockets are bound
     // Set up TCP first to ensure transport creates/binds both TCP and UDP sockets
    if (!server->setupTcpSocket([]() { return std::make_unique<uds::UdsServerModel>(); })) {
        console->critical("Failed to set up TCP socket");
        return 1;
    }

    // Start announcement thread after sockets are bound
    if (!server->setupUdpSocket()) {
        console->critical("Failed to set up UDP announcements");
        server->stop(); // Clean up before exiting
        return 1;
    }

    console->info("DoIP UDP Server is running");

    while (server->isRunning()) {
        // Main loop can perform other tasks or just sleep
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}