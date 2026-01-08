#include "tp/MockServerTransport.h"
#include "tp/MockConnectionTransport.h"
#include "DoIPMessage.h"
#include "DoIPPayloadType.h"
#include <doctest/doctest.h>

using namespace doip;

TEST_SUITE("MockServerTransport") {
    TEST_CASE("MockServerTransport construction and basic properties") {
        SUBCASE("Default identifier") {
            MockServerTransport transport;

            CHECK_FALSE(transport.isActive());
            CHECK(transport.getIdentifier() == "mock-server:0");
        }

        SUBCASE("Custom identifier") {
            MockServerTransport transport("custom-test-server");

            CHECK_FALSE(transport.isActive());
            CHECK(transport.getIdentifier() == "custom-test-server:0");
        }
    }

    TEST_CASE("MockServerTransport setup") {
        MockServerTransport transport("test-server");

        SUBCASE("Setup activates transport and sets port") {
            bool result = transport.setup(13400);

            CHECK(result);
            CHECK(transport.isActive());
            CHECK(transport.getIdentifier() == "test-server:13400");
        }

        SUBCASE("Setup with different ports") {
            transport.setup(8080);
            CHECK(transport.getIdentifier() == "test-server:8080");

            transport.setup(3000);
            CHECK(transport.getIdentifier() == "test-server:3000");
        }

        SUBCASE("Setup always succeeds") {
            // Mock transport should always succeed setup
            CHECK(transport.setup(0));
            CHECK(transport.setup(65535));
        }
    }

    TEST_CASE("MockServerTransport acceptConnection without injected connections") {
        MockServerTransport transport("test-server");
        transport.setup(13400);

        SUBCASE("Returns nullptr when no connections queued") {
            auto conn = transport.acceptConnection();
            CHECK(conn == nullptr);
        }

        SUBCASE("Multiple calls return nullptr when queue is empty") {
            CHECK(transport.acceptConnection() == nullptr);
            CHECK(transport.acceptConnection() == nullptr);
            CHECK(transport.acceptConnection() == nullptr);
        }
    }

    TEST_CASE("MockServerTransport acceptConnection with injected connections") {
        MockServerTransport transport("test-server");
        transport.setup(13400);

        SUBCASE("Single injected connection") {
            auto mockConn = std::make_unique<MockConnectionTransport>("client-1");
            transport.injectConnection(std::move(mockConn));

            auto conn = transport.acceptConnection();
            REQUIRE(conn != nullptr);
            CHECK(conn->getIdentifier() == "client-1");

            // Queue should be empty now
            CHECK(transport.acceptConnection() == nullptr);
        }

        SUBCASE("Multiple injected connections are returned in order") {
            transport.injectConnection(std::make_unique<MockConnectionTransport>("client-1"));
            transport.injectConnection(std::make_unique<MockConnectionTransport>("client-2"));
            transport.injectConnection(std::make_unique<MockConnectionTransport>("client-3"));

            auto conn1 = transport.acceptConnection();
            REQUIRE(conn1 != nullptr);
            CHECK(conn1->getIdentifier() == "client-1");

            auto conn2 = transport.acceptConnection();
            REQUIRE(conn2 != nullptr);
            CHECK(conn2->getIdentifier() == "client-2");

            auto conn3 = transport.acceptConnection();
            REQUIRE(conn3 != nullptr);
            CHECK(conn3->getIdentifier() == "client-3");

            // Queue should be empty now
            CHECK(transport.acceptConnection() == nullptr);
        }

        SUBCASE("Inject connections after accepting some") {
            transport.injectConnection(std::make_unique<MockConnectionTransport>("client-1"));

            auto conn1 = transport.acceptConnection();
            REQUIRE(conn1 != nullptr);
            CHECK(conn1->getIdentifier() == "client-1");

            // Inject another connection
            transport.injectConnection(std::make_unique<MockConnectionTransport>("client-2"));

            auto conn2 = transport.acceptConnection();
            REQUIRE(conn2 != nullptr);
            CHECK(conn2->getIdentifier() == "client-2");
        }
    }

    TEST_CASE("MockServerTransport close functionality") {
        MockServerTransport transport("test-server");
        transport.setup(13400);

        SUBCASE("Close deactivates transport") {
            CHECK(transport.isActive());

            transport.close();

            CHECK_FALSE(transport.isActive());
        }

        SUBCASE("Close clears connection queue") {
            transport.injectConnection(std::make_unique<MockConnectionTransport>("client-1"));
            transport.injectConnection(std::make_unique<MockConnectionTransport>("client-2"));

            transport.close();

            // After close, queue should be cleared
            CHECK(transport.acceptConnection() == nullptr);
        }

        SUBCASE("acceptConnection returns nullptr after close") {
            transport.injectConnection(std::make_unique<MockConnectionTransport>("client-1"));

            transport.close();

            auto conn = transport.acceptConnection();
            CHECK(conn == nullptr);
        }

        SUBCASE("Can close multiple times") {
            transport.close();
            CHECK_FALSE(transport.isActive());

            transport.close(); // Second close should be safe
            CHECK_FALSE(transport.isActive());
        }
    }

    TEST_CASE("MockServerTransport isActive state transitions") {
        MockServerTransport transport("test-server");

        SUBCASE("Initial state is inactive") {
            CHECK_FALSE(transport.isActive());
        }

        SUBCASE("Active after setup") {
            transport.setup(13400);
            CHECK(transport.isActive());
        }

        SUBCASE("Inactive after close") {
            transport.setup(13400);
            transport.close();
            CHECK_FALSE(transport.isActive());
        }

        SUBCASE("Can reactivate after close by calling setup again") {
            transport.setup(13400);
            CHECK(transport.isActive());

            transport.close();
            CHECK_FALSE(transport.isActive());

            transport.setup(8080);
            CHECK(transport.isActive());
        }
    }

    TEST_CASE("MockServerTransport getIdentifier formatting") {
        SUBCASE("Identifier includes port") {
            MockServerTransport transport("my-server");
            transport.setup(12345);

            CHECK(transport.getIdentifier() == "my-server:12345");
        }

        SUBCASE("Identifier updates when port changes") {
            MockServerTransport transport("server");

            CHECK(transport.getIdentifier() == "server:0");

            transport.setup(8080);
            CHECK(transport.getIdentifier() == "server:8080");

            transport.setup(9090);
            CHECK(transport.getIdentifier() == "server:9090");
        }

        SUBCASE("Identifier with special characters") {
            MockServerTransport transport("test-server_v2.0");
            transport.setup(3000);

            CHECK(transport.getIdentifier() == "test-server_v2.0:3000");
        }
    }

    TEST_CASE("MockServerTransport clearQueues via close") {
        MockServerTransport transport("test-server");
        transport.setup(13400);

        SUBCASE("clearQueues is called on close") {
            // Inject multiple connections
            transport.injectConnection(std::make_unique<MockConnectionTransport>("client-1"));
            transport.injectConnection(std::make_unique<MockConnectionTransport>("client-2"));
            transport.injectConnection(std::make_unique<MockConnectionTransport>("client-3"));

            // Close should clear all queues
            transport.close();

            // Reactivate and verify queues are empty
            transport.setup(13400);
            CHECK(transport.acceptConnection() == nullptr);
        }
    }

    TEST_CASE("MockServerTransport thread safety simulation") {
        MockServerTransport transport("test-server");
        transport.setup(13400);

        SUBCASE("Inject and accept in sequence") {
            // Simulate multiple inject/accept cycles
            for (int i = 0; i < 10; ++i) {
                std::string clientName = "client-" + std::to_string(i);
                transport.injectConnection(std::make_unique<MockConnectionTransport>(clientName));

                auto conn = transport.acceptConnection();
                REQUIRE(conn != nullptr);
                CHECK(conn->getIdentifier() == clientName);
            }
        }
    }

    TEST_CASE("MockServerTransport integration with MockConnectionTransport") {
        MockServerTransport transport("test-server");
        transport.setup(13400);

        SUBCASE("Accepted connection is usable") {
            auto mockConn = std::make_unique<MockConnectionTransport>("client-1");

            // Inject a message into the connection before injecting it
            DoIPMessage testMsg(DoIPPayloadType::VehicleIdentificationRequest, nullptr, 0);
            mockConn->injectMessage(testMsg);

            transport.injectConnection(std::move(mockConn));

            // Accept the connection
            auto conn = transport.acceptConnection();
            REQUIRE(conn != nullptr);

            // Verify we can receive the injected message through the connection
            auto receivedMsg = conn->receiveMessage();
            REQUIRE(receivedMsg.has_value());
            CHECK(receivedMsg->getPayloadType() == DoIPPayloadType::VehicleIdentificationRequest);
        }

        SUBCASE("Send message through accepted connection") {
            transport.injectConnection(std::make_unique<MockConnectionTransport>("client-1"));

            auto conn = transport.acceptConnection();
            REQUIRE(conn != nullptr);

            // Send a message through the connection
            DoIPMessage msg(DoIPPayloadType::RoutingActivationResponse, nullptr, 0);
            ssize_t sent = conn->sendMessage(msg);

            CHECK(sent == static_cast<ssize_t>(msg.size()));
            CHECK(conn->isActive());
        }
    }

    TEST_CASE("MockServerTransport edge cases") {
        SUBCASE("Accept before setup") {
            MockServerTransport transport("test-server");

            // Transport is not active, should return nullptr
            auto conn = transport.acceptConnection();
            CHECK(conn == nullptr);
        }

        SUBCASE("Setup with port 0") {
            MockServerTransport transport("test-server");
            transport.setup(0);

            CHECK(transport.isActive());
            CHECK(transport.getIdentifier() == "test-server:0");
        }

        SUBCASE("Empty identifier") {
            MockServerTransport transport("");
            transport.setup(13400);

            CHECK(transport.getIdentifier() == ":13400");
        }

        SUBCASE("Inject null behavior prevention") {
            MockServerTransport transport("test-server");
            transport.setup(13400);

            // This tests the queue behavior when popping from empty queue
            // after injecting and accepting a connection
            transport.injectConnection(std::make_unique<MockConnectionTransport>("client-1"));
            auto conn = transport.acceptConnection();
            REQUIRE(conn != nullptr);

            // Try to accept again - should return nullptr
            CHECK(transport.acceptConnection() == nullptr);
        }
    }
}
