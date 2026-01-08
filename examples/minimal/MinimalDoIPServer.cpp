#include "DoIPServer.h"
#include "uds/UdsServerModel.h"

using namespace doip;
using doip::uds::UdsServerModel;

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    ServerConfig cfg;

    auto server = std::make_unique<DoIPServer>(cfg);
    if (!server->setupTcpSocket([]() { return std::make_unique<UdsServerModel>(); })) {
        std::cerr << "Failed to setup DoIP TCP server" << std::endl;
        return 1;
    }

    std::cout << "DoIP Minimal Server is running" << std::endl;

    while (server->isTcpRunning()) {
        // Main loop can perform other tasks or just sleep
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}