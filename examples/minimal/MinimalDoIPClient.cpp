#include <iostream>

#include "DoIPClient.h"

using namespace doip;

struct  MyDoIPClientModel : public DoIPClientModel {
    void routingActivated(DoIPClient& client, bool activated, DoIPAddress logicalAddress) override {
        (void)client;
        if (activated) {
            std::cout << "Routing activated with logical address: " << logicalAddress << std::endl;
        } else {
            std::cout << "Routing activation failed" << std::endl;
        }
        client.sendDiagnosticMessage({0x10, 0x02}); // Example UDS message
    }

    void messageSent(DoIPClient& client, const DoIPMessage& msg) override {
        (void)client;
        std::cout << "Message sent: " << msg << std::endl;
    }

    bool diagMessageReceived(DoIPClient& client, const DoIPMessage& msg) override {
        (void)client;
        std::cout << "Message received: " << msg << std::endl;

        return false; // return false to indicate quit
    }
};


int main() {
    DoIPClient client(std::make_unique<MyDoIPClientModel>());
    if (!client.startTcpConnection()) {
        std::cerr << "Failed to start TCP connection to DoIP server" << std::endl;
        return 1;
    }

    // Wait a bit for communication to complete
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // Clean up: close the connection (this will join the thread)
    client.closeTcpConnection();

    return 0;
}