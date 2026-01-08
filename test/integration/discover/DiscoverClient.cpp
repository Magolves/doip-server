#include "DoIPClient.h"
#include "util/Logger.h"

using namespace doip;
using std::string;


DoIPClient client;

int main() {
    string serverAddress = "127.0.0.1"; // Default to loopback for testing to same hosts
    auto console = spdlog::stdout_color_mt("discover-client");

    console->info("Starting DoIP Client");

    // Start UDP connections (don't start TCP yet)
    if (!client.startUdpConnection()) {
        console->error("Failed to start UDP connection");
        return EXIT_FAILURE;
    }
    client.startAnnouncementListener(); // Listen for Vehicle Announcements on port 13401

    // Listen for Vehicle Announcements first
    console->info("Listening for Vehicle Announcements...");
    if (!client.receiveVehicleAnnouncement()) {
        console->warn("No Vehicle Announcement received");
        return EXIT_FAILURE;
    }

    ServerProperties vehicleInfo = client.getServerProperties();
    console->info("Discovered Vehicle - VIN: {}, EID: {}, GID: {}, Logical Address: 0x{:04X}",
                  fmt::streamed(vehicleInfo.vin),
                  fmt::streamed(vehicleInfo.eid),
                  fmt::streamed(vehicleInfo.gid),
                  vehicleInfo.logicalAddress);

    // Now start TCP connection for diagnostic communication
    console->info("Discovery complete, closing UDP connections");
    client.closeUdpConnection();
    doip::Logger::shutdown();
    return 0;
}
