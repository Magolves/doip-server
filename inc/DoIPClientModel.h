#ifndef DOIPCLIENTMODEL_H
#define DOIPCLIENTMODEL_H

#include "DoIPMessage.h"

#include <memory>
#include <iostream>

namespace doip {
    class DoIPClient;


    struct DoIPClientModel {
        virtual ~DoIPClientModel() = default;

        /**
         * @brief Called when routing activation has finished - either successfully or failed
         *
         * @param client the DoIP client
         * @param activated true if routing was activated successfully, false otherwise
         * @param logicalAddress the assigned logical address (if activated)
         */
        virtual void routingActivationFinished(DoIPClient& client, bool activated, DoIPAddress logicalAddress = ZERO_ADDRESS) {
            (void)client;
            (void)activated;
            (void)logicalAddress;
        }

        /**
         * @brief Called when a diagnostic message has been acknowledged by the server. This
         * includes both positive and negative acknowledgments.
         *
         * @param client the DoIP client
         * @param ack the diagnostic acknowledgment received
         */
        virtual void diagMessageAcked(DoIPClient& client, DoIPDiagnosticAck ack) {
            (void)client;
            (void)ack;
        }

        /**
         * @brief Called when a diagnostic message has been received from the server.
         *
         * @param client the DoIP client
         * @param msg the diagnostic message received
         */
        virtual void diagMessageReceived(DoIPClient& client, const DoIPMessage& msg) {
            (void)client;
            (void)msg;
        }

        /**
         * @brief Called when a DoIP message has been sent to the server.
         *
         * @param client the DoIP client
         * @param msg the DoIP message that was sent
         */
        virtual void messageSent(DoIPClient& client, const DoIPMessage& msg) {
            (void)client;
            (void)msg;
        }

        /**
         * @brief Called when an error occurs in the DoIP client.
         *
         * @param client the DoIP client
         * @param errorMsg the error message
         */
        virtual void error(DoIPClient& client, const std::string& errorMsg) {
            (void)client;
            std::cerr << "DoIPClientModel Error: " << errorMsg << std::endl;
        }

        /**
         * @brief Called when a vehicle announcement message is received.
         * This callback is only used if the DoIP client is listening for
         * vehicle announcements on UDP.
         *
         * @param client the DoIP client
         * @param msg the vehicle announcement message received
         * @param fromAddress the IP address the announcement was received from
         */
        virtual void vehicleAnnouncementReceived(DoIPClient& client, const DoIPMessage& msg, const std::string &fromAddress) {
            (void)client;
            (void)msg;
            (void)fromAddress;
        }
    };

    using UniqueDoIPClientModelPtr = std::unique_ptr<DoIPClientModel>;
} // namespace doip

#endif /* DOIPCLIENTMODEL_H */
