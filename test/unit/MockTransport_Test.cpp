#include "tp/MockConnectionTransport.h"
#include "DoIPMessage.h"
#include "DoIPPayloadType.h"
#include <doctest/doctest.h>

using namespace doip;

TEST_SUITE("MockConnectionTransport") {
    TEST_CASE("MockConnectionTransport basic send/receive") {
        MockConnectionTransport transport("test-transport");

        REQUIRE(transport.isActive());
        REQUIRE(transport.getIdentifier() == "test-transport");
        REQUIRE_FALSE(transport.hasSentMessages());

        SUBCASE("Send message and verify in sent queue") {
            DoIPMessage msg(DoIPPayloadType::VehicleIdentificationRequest, nullptr, 0);
            ssize_t sent = transport.sendMessage(msg);

            CHECK(sent == static_cast<ssize_t>(msg.size()));
            CHECK(transport.hasSentMessages());
            CHECK(transport.sentMessageCount() == 1);

            auto sentMsg = transport.popSentMessage();
            REQUIRE(sentMsg.has_value());
            CHECK(sentMsg->getPayloadType() == DoIPPayloadType::VehicleIdentificationRequest);
            CHECK_FALSE(transport.hasSentMessages());
        }

        SUBCASE("Inject message and receive it") {
            DoIPMessage injected(DoIPPayloadType::RoutingActivationRequest, nullptr, 0);
            transport.injectMessage(injected);

            auto received = transport.receiveMessage();
            REQUIRE(received.has_value());
            CHECK(received->getPayloadType() == DoIPPayloadType::RoutingActivationRequest);

            // Queue should be empty now
            auto empty = transport.receiveMessage();
            CHECK_FALSE(empty.has_value());
        }

        SUBCASE("Close transport") {
            transport.close(DoIPCloseReason::ApplicationRequest);
            CHECK_FALSE(transport.isActive());

            DoIPMessage msg(DoIPPayloadType::AliveCheckRequest, nullptr, 0);
            ssize_t sent = transport.sendMessage(msg);
            CHECK(sent == -1); // Should fail on closed transport

            auto received = transport.receiveMessage();
            CHECK_FALSE(received.has_value());
        }

        SUBCASE("Clear queues") {
            // Send some messages
            for (int i = 0; i < 3; ++i) {
                DoIPMessage msg(DoIPPayloadType::VehicleIdentificationRequest, nullptr, 0);
                transport.sendMessage(msg);
            }

            // Inject some messages
            for (int i = 0; i < 2; ++i) {
                DoIPMessage msg(DoIPPayloadType::RoutingActivationRequest, nullptr, 0);
                transport.injectMessage(msg);
            }

            CHECK(transport.sentMessageCount() == 3);

            transport.clearQueues();

            CHECK(transport.sentMessageCount() == 0);
            CHECK_FALSE(transport.hasSentMessages());
            auto received = transport.receiveMessage();
            CHECK_FALSE(received.has_value());
        }
    }

    TEST_CASE("MockConnectionTransport bidirectional communication simulation") {
        MockConnectionTransport transport("client-mock");

        // Simulate client sending routing activation request
        uint8_t payload[] = {0x00, 0x01, 0x02, 0x03}; // Dummy payload
        DoIPMessage request(DoIPPayloadType::RoutingActivationRequest, payload, sizeof(payload));
        transport.sendMessage(request);

        // Verify message was sent
        auto sentMsg = transport.popSentMessage();
        REQUIRE(sentMsg.has_value());
        CHECK(sentMsg->getPayloadType() == DoIPPayloadType::RoutingActivationRequest);

        // Simulate server response
        DoIPMessage response(DoIPPayloadType::RoutingActivationResponse, payload, sizeof(payload));
        transport.injectMessage(response);

        // Client receives response
        auto receivedMsg = transport.receiveMessage();
        REQUIRE(receivedMsg.has_value());
        CHECK(receivedMsg->getPayloadType() == DoIPPayloadType::RoutingActivationResponse);
    }
}
