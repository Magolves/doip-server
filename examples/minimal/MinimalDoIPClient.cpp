#include <iostream>

#include "DoIPClient.h"

using namespace doip;

const std::vector<ByteArray> UDS_MESSAGES = {
    {0x10, 0x02},       // open diag session
    {0x22, 0xF1, 0x90}, // read VIN
    {0x10, 0x01},       // close diag session
};

/**
 * @brief Example DoIP client model that handles callbacks.
 *
 */
struct MyDoIPClientModel : public DoIPClientModel {
    void routingActivationFinished(DoIPClient &client, bool activated, DoIPAddress logicalAddress) override {
        (void)client;
        if (activated) {
            std::cout << "Routing activated with logical address: " << logicalAddress << std::endl;
            sendNextUdsMessage(client);
        } else {
            std::cout << "Routing activation failed" << std::endl;
        }
    }

    void messageSent(DoIPClient &client, const DoIPMessage &msg) override {
        (void)client;
        std::cout << "Message sent: " << msg << std::endl;
    }

    void diagMessageAcked(DoIPClient &client, DoIPDiagnosticAck ack) override {
        (void)client;
        std::cout << "Diagnostic message acknowledged with: " << static_cast<int>(ack) << std::endl;
    }

    CallbackResult diagMessageReceived(DoIPClient &client, const DoIPMessage &msg) override {
        (void)client;
        std::cout << "Message received: " << msg << std::endl;
        std::cout << "--> index: "  << udsMessageIndex << std::endl;
        return sendNextUdsMessage(client);
    }

    void error(DoIPClient &client, const std::string &errorMsg) override {
        (void)client;
        std::cerr << "Error: " << errorMsg << std::endl;
    }

    private:
        size_t udsMessageIndex = 0;

        CallbackResult sendNextUdsMessage(DoIPClient &client) {
            if (udsMessageIndex < UDS_MESSAGES.size()) {
                std::cout << "Sending UDS message " << (udsMessageIndex + 1) << "/" << UDS_MESSAGES.size() << std::endl;
                client.sendDiagnosticMessage(UDS_MESSAGES[udsMessageIndex]);
            }
            // wait until the last message has been received
            udsMessageIndex++;
            return (udsMessageIndex <= UDS_MESSAGES.size()) ? CallbackResult::Continue : CallbackResult::Stop;
        }

        // 0 -> 1
};

int main() {
    DoIPClient client(std::make_unique<MyDoIPClientModel>());
    if (!client.startTcpConnection()) {
        std::cerr << "Failed to start TCP connection to DoIP server" << std::endl;
        return 1;
    }

    // Wait a bit for communication to complete
    while (client.isTcpRunning()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Clean up: close the connection (this will join the thread)
    client.closeTcpConnection();

    return 0;
}