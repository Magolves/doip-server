#include <doctest/doctest.h>
#include <iostream>
#include <sstream>
#include <vector>

#include "DoIPAddress.h"
#include "DoIPConnection.h"
#include "MockConnectionTransport.h"

using namespace doip;

/**
 * @brief Miscellaneous tests for various components where a dedicated module is not justified
 *
 */

TEST_SUITE("DoIPAddress") {
    TEST_CASE("Zero address") {
        DoIPAddress zeroAddr = readAddressFrom(nullptr, 0);
        CHECK(zeroAddr == DoIPAddress(0x0));
    }

    TEST_CASE("Valid source address") {
        const uint8_t validData[] = { 0xE0, 0x10 }; // 0xE010 is a valid source address
        const uint8_t invalidData[] = { 0xD0, 0x10 }; // 0xD010 is NOT a valid source address

        CHECK(isValidSourceAddress(validData, 0) == true);
        CHECK(isValidSourceAddress(invalidData, 0) == false);
    }
}

TEST_SUITE("DoIPConnection") {
    TEST_CASE("Connection Initialization") {

        SharedTimerManagerPtr<ConnectionTimers> timerManager = std::make_shared<TimerManager<ConnectionTimers>>();
        // Create connection with mock transport (simulates closed socket)
        auto mockTransport = std::make_unique<MockConnectionTransport>();
        DoIPConnection conn(std::move(mockTransport), std::make_unique<DefaultDoIPServerModel>(), timerManager);

        // Connection should be active after construction
        CHECK(conn.isSocketActive() == true);

        // After closing, socket should be inactive
        conn.closeConnection(DoIPCloseReason::ApplicationRequest);
        CHECK(conn.isSocketActive() == false);
    }
}