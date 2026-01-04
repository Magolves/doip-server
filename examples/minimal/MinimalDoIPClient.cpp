#include <iostream>

#include "DoIPClient.h"

using namespace doip;

struct  MyDoIPClientModel : public DoIPClientModel {
    void routingActivationFinished(DoIPClient& client, bool activated, DoIPAddress logicalAddress) override {
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

    void diagMessageAcked(DoIPClient& client, DoIPDiagnosticAck ack) override {
        (void)client;
        std::cout << "Diagnostic message acknowledged with: " << static_cast<int>(ack) << std::endl;
    }

    void diagMessageReceived(DoIPClient& client, const DoIPMessage& msg) override {
        (void)client;
        std::cout << "Message received: " << msg << std::endl;
    }

    void error(DoIPClient& client, const std::string& errorMsg) override {
        (void)client;
        std::cerr << "Error: " << errorMsg << std::endl;
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