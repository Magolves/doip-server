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
 * It illustrates the usage of the DoIPClientModel interface.
 */
struct MyDoIPClientModel : public DoIPClientModel {
    /**
     * @brief Callback when routing activation has finished.
     * @param client the DoIP client
     * @param activated true if routing was activated successfully, false otherwise
     * @param logicalAddress the assigned logical address (if activated)
     */
    void routingActivationFinished(DoIPClient &client, bool activated, DoIPAddress logicalAddress) override {
        (void)client;
        if (activated) {
            std::cout << "Routing activated with logical address: " << logicalAddress << std::endl;
            sendNextUdsMessage(client);
        } else {
            std::cout << "Routing activation failed" << std::endl;
        }
    }

    /**
     * @brief Callback when a DoIP message has been sent to the server.
     * @param client the DoIP client
     * @param msg the DoIP message that was sent
    */
    void messageSent(DoIPClient &client, const DoIPMessage &msg) override {
        (void)client;
        std::cout << "Message sent: " << msg << std::endl;
    }

    /**
     * @brief Callback when a diagnostic message has been acknowledged by the server.
     * Payload types for acknowledgments are DiagnosticMessagePositiveAck (0x8002) and DiagnosticMessageNegativeAck (0x8003).
     * @param client the DoIP client
     * @param ack the diagnostic acknowledgment received
     */
    void diagMessageAcked(DoIPClient &client, DoIPDiagnosticAck ack) override {
        (void)client;
        std::cout << "Diagnostic message acknowledged with: " << static_cast<int>(ack) << std::endl;
    }

    /**
     * @brief Callback when a diagnostic message has (0x8001) been received from the server.
     *
     * @param client the DoIP client
     * @param msg the diagnostic message received
     * @return CallbackResult
     */
    CallbackResult diagMessageReceived(DoIPClient &client, const DoIPMessage &msg) override {
        (void)client;
        std::cout << "Message received: " << msg << std::endl;
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
                client.sendDiagnosticMessage(UDS_MESSAGES[udsMessageIndex]);
            }
            // wait until the last message has been received
            udsMessageIndex++;
            return (udsMessageIndex <= UDS_MESSAGES.size()) ? CallbackResult::Continue : CallbackResult::Stop;
        }
};

int main() {
    // Assign the client model
    DoIPClient client(std::make_unique<MyDoIPClientModel>());
    // Setup TCP connection to DoIP server 
    if (!client.startTcpConnection()) {
        std::cerr << "Failed to start TCP connection to DoIP server" << std::endl;
        return 1;
    }

    while (client.isTcpRunning()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    client.closeTcpConnection();

    return 0;
}