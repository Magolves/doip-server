#include "DoIPServer.h"

using namespace doip;

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    ServerConfig cfg;
    auto console = spdlog::stdout_color_mt("doip-server");

    console->info("Starting DoIP Minimal Server");

    auto server = std::make_unique<DoIPServer>(cfg);
    if (!server->setupTcpSocket()) {
        console->error("Failed to start DoIP Discovery Server");
        return 1;
    }

    console->info("DoIP Minimal Server is running");

    while (server->isRunning()) {
        // Main loop can perform other tasks or just sleep
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}