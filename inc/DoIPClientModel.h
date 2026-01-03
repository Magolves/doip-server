#ifndef DOIPCLIENTMODEL_H
#define DOIPCLIENTMODEL_H

#include "DoIPMessage.h"

#include <memory>

namespace doip {
    class DoIPClient;


    struct DoIPClientModel {
        virtual ~DoIPClientModel() = default;

        // TODO: Add UDP methods

        virtual void routingActivated(DoIPClient& client, bool activated, DoIPAddress logicalAddress = ZERO_ADDRESS) {
            (void)client;
            (void)activated;
            (void)logicalAddress;
        }

        virtual bool messageReceived(DoIPClient& client, const DoIPMessage& msg) {
            (void)client;
            (void)msg;
            return false; // return false to indicate quit
        }

        virtual void messageSent(DoIPClient& client, const DoIPMessage& msg) {
            (void)client;
            (void)msg;
        }
    };

    using UniqueDoIPClientModelPtr = std::unique_ptr<DoIPClientModel>;
} // namespace doip

#endif /* DOIPCLIENTMODEL_H */
