#include "DoIPServer.h"
#include "uds/UdsServerModel.h"

using namespace doip;
using doip::uds::UdsServerModel;

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    ServerConfig cfg;
    cfg.loopback = true; // For testing, use loopback announcements
    // Set server properties
    cfg.properties.logicalAddress = DoIPAddress(0x29); // Example logical address
    cfg.properties.vin = Vin("WVWZZZ1JZ3W386752"); // Some valid VIN
    cfg.properties.eid = EntityId(0x654321);
    cfg.properties.gid = GroupId(0x123456);

    auto server = std::make_unique<DoIPServer>(cfg);
    auto console = spdlog::stdout_color_mt("doip-udp-server");

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