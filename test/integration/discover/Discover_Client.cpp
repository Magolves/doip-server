#include "DoIPClient.h"
#include "DoIPMessage.h"
#include "Logger.h"

#include <iomanip>
#include <iostream>
#include <thread>

using namespace doip;
using std::string;
using std::cout;

DoIPClient client;

int main() {
    // string serverAddress = "224.0.0.2"; // Default multicast address
    string serverAddress = "127.0.0.1"; // Default to loopback for testing
    auto console = spdlog::stdout_color_mt("discover-client");

    console->info("Starting DoIP Client");

    // Start UDP connections (don't start TCP yet)
    client.startUdpConnection();
    client.startAnnouncementListener(); // Listen for Vehicle Announcements on port 13401

    // Listen for Vehicle Announcements first
    console->info("Listening for Vehicle Announcements...");
    if (!client.receiveVehicleAnnouncement()) {
        console->warn("No Vehicle Announcement received");
        return EXIT_FAILURE;
    }

    client.printVehicleInformationResponse();

    // Send Vehicle Identification Request to configured address
    if (client.sendVehicleIdentificationRequest(serverAddress.c_str()) > 0) {
        console->info("Vehicle Identification Request sent successfully");
        client.receiveUdpMessage();
    }

    // Now start TCP connection for diagnostic communication
    console->info("Discovery complete, closing UDP connections");
    client.closeUdpConnection();
    doip::Logger::shutdown();
    return 0;
}
