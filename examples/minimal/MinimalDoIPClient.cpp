#include <iostream>

#include "DoIPClient.h"

using namespace doip;

struct  MyDoIPClientModel : public DoIPClientModel {
    void messageReceived(DoIPClient& client, const DoIPMessage& msg) override {
        (void)client;
        std::cout << "Message received: " << msg << std::endl;
    }
};


int main() {
    DoIPClient client(std::make_unique<MyDoIPClientModel>());
    if (!client.startTcpConnection()) {
        std::cerr << "Failed to start TCP connection to DoIP server" << std::endl;
        return 1;
    }

    while(client.isTcpConnected()) {
        // Main loop can perform other tasks or just sleep
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

}