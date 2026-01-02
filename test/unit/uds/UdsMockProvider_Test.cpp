#include <doctest/doctest.h>
#include <memory>

#include "../doctest_aux.h"
#include "uds/UdsMockProvider.h"
#include "uds/UdsDefaultModel.h"
#include "uds/IUdsModel.h"

using namespace doip;
using namespace doip::uds;

struct UdsMockProviderTestModel : UdsDefaultModel {
    UdsMockProviderTestModel() : UdsDefaultModel() {}

    std::string_view getModelName() const override {
        return "UdsMockProviderTestModel";
    }
};

TEST_SUITE("UdsMockProvider") {
    struct UdsMockProviderFixture {
        UdsMockProvider provider{std::make_unique<UdsMockProviderTestModel>()};

        UdsMockProviderFixture() = default;
        ~UdsMockProviderFixture() = default;
    };

    TEST_CASE_FIXTURE(UdsMockProviderFixture, "getProviderName returns correct name") {
        CHECK(provider.getProviderName() == "UdsMockProvider");
    }

    TEST_CASE_FIXTURE(UdsMockProviderFixture, "sendRequest with null callback does not crash") {
        ByteArray request = {0x10, 0x01};
        provider.sendRequest(request, nullptr);
        // Test passes if no crash occurs
    }

    TEST_CASE_FIXTURE(UdsMockProviderFixture, "sendRequest invokes callback exactly once") {
        ByteArray request = {0x10, 0x01};
        int callbackCount = 0;

        provider.sendRequest(request, [&callbackCount](const DownstreamResponse &) noexcept {
            callbackCount++;
        });

        CHECK(callbackCount == 1);
    }

    TEST_CASE_FIXTURE(UdsMockProviderFixture, "sendRequest returns Handled status") {
        ByteArray request = {0x10, 0x01};
        DownstreamStatus receivedStatus = DownstreamStatus::Error;

        provider.sendRequest(request, [&receivedStatus](const DownstreamResponse &response) noexcept {
            receivedStatus = response.status;
        });

        CHECK(receivedStatus == DownstreamStatus::Handled);
    }

    TEST_CASE_FIXTURE(UdsMockProviderFixture, "sendRequest returns non-negative latency") {
        ByteArray request = {0x10, 0x01};
        std::chrono::milliseconds receivedLatency{-1};

        provider.sendRequest(request, [&receivedLatency](const DownstreamResponse &response) noexcept {
            receivedLatency = response.latency;
        });

        CHECK(receivedLatency.count() >= 0);
    }

    TEST_CASE_FIXTURE(UdsMockProviderFixture, "sendRequest handles invalid service with negative response") {
        ByteArray request = {0x00, 0x00}; // Invalid UDS service ID
        ByteArray expectedResponse = {0x7F, 0x00, 0x11}; // NRC: ServiceNotSupported
        ByteArray receivedPayload;

        provider.sendRequest(request, [&receivedPayload](const DownstreamResponse &response) noexcept {
            receivedPayload = response.payload;
        });

        CHECK_BYTE_ARRAY_EQ(receivedPayload, expectedResponse);
    }

    TEST_CASE_FIXTURE(UdsMockProviderFixture, "sendRequest handles DiagnosticSessionControl session change") {
        // Request ExtendedDiagnosticSession (0x03) - different from default (0x01)
        // This should succeed since we're changing from default to extended session
        ByteArray request = {0x10, 0x03};
        ByteArray receivedPayload;

        provider.sendRequest(request, [&receivedPayload](const DownstreamResponse &response) noexcept {
            receivedPayload = response.payload;
        });

        // Positive response SID = 0x10 + 0x40 = 0x50
        CHECK(receivedPayload.size() > 0);
        CHECK(receivedPayload[0] == 0x50);
        CHECK(receivedPayload[1] == 0x03); // Session type echoed
    }

    TEST_CASE_FIXTURE(UdsMockProviderFixture, "sendRequest handles DiagnosticSessionControl same session with NRC") {
        // Request DefaultSession (0x01) when already in DefaultSession
        // This should return ConditionsNotCorrect (0x22) per the model implementation
        ByteArray request = {0x10, 0x01};
        ByteArray receivedPayload;

        provider.sendRequest(request, [&receivedPayload](const DownstreamResponse &response) noexcept {
            receivedPayload = response.payload;
        });

        // Negative response: 0x7F, SID, NRC
        CHECK(receivedPayload.size() == 3);
        CHECK(receivedPayload[0] == 0x7F);
        CHECK(receivedPayload[1] == 0x10);
        CHECK(receivedPayload[2] == 0x22); // ConditionsNotCorrect
    }

    TEST_CASE_FIXTURE(UdsMockProviderFixture, "sendRequest handles ReadDataByIdentifier for VIN") {
        // RDBI request for VIN (DID 0xF190)
        ByteArray request = {0x22, 0xF1, 0x90};
        ByteArray receivedPayload;

        provider.sendRequest(request, [&receivedPayload](const DownstreamResponse &response) noexcept {
            receivedPayload = response.payload;
        });

        // Expect positive response: 0x62 (RDBI positive response)
        CHECK(receivedPayload.size() > 0);
        CHECK(receivedPayload[0] == 0x62);

        // Should contain the DID echoed back
        if (receivedPayload.size() >= 3) {
            uint16_t did = receivedPayload.readU16(1);
            CHECK(did == 0xF190);
        }
    }

    TEST_CASE_FIXTURE(UdsMockProviderFixture, "sendRequest handles empty request with empty response") {
        ByteArray request = {};
        ByteArray receivedPayload;

        provider.sendRequest(request, [&receivedPayload](const DownstreamResponse &response) noexcept {
            receivedPayload = response.payload;
        });

        // Empty request returns empty response per UdsMock::handleDiagnosticRequest
        CHECK(receivedPayload.empty());
    }

    TEST_CASE("UdsMockProvider default constructor uses UdsDefaultModel") {
        UdsMockProvider defaultProvider;

        ByteArray request = {0x10, 0x01};
        DownstreamStatus receivedStatus = DownstreamStatus::Error;

        defaultProvider.sendRequest(request, [&receivedStatus](const DownstreamResponse &response) noexcept {
            receivedStatus = response.status;
        });

        CHECK(receivedStatus == DownstreamStatus::Handled);
    }

    TEST_CASE("UdsMockProvider with custom model processes requests correctly") {
        auto customModel = std::make_unique<UdsMockProviderTestModel>();
        UdsMockProvider customProvider(std::move(customModel));

        // Request ExtendedDiagnosticSession (0x03) to get a positive response
        ByteArray request = {0x10, 0x03};
        ByteArray receivedPayload;

        customProvider.sendRequest(request, [&receivedPayload](const DownstreamResponse &response) noexcept {
            receivedPayload = response.payload;
        });

        CHECK(receivedPayload.size() > 0);
        CHECK(receivedPayload[0] == 0x50); // Positive response for DSC
    }

    TEST_CASE_FIXTURE(UdsMockProviderFixture, "sendRequest handles TesterPresent service") {
        ByteArray request = {0x3E, 0x00}; // TesterPresent with subfunction 0x00
        ByteArray receivedPayload;

        provider.sendRequest(request, [&receivedPayload](const DownstreamResponse &response) noexcept {
            receivedPayload = response.payload;
        });

        // TesterPresent positive response: 0x7E, 0x00
        CHECK(receivedPayload.size() >= 2);
        CHECK(receivedPayload[0] == 0x7E);
        CHECK(receivedPayload[1] == 0x00);
    }

    TEST_CASE_FIXTURE(UdsMockProviderFixture, "sendRequest handles ECUReset service") {
        ByteArray request = {0x11, 0x01}; // ECUReset - Hard Reset
        ByteArray receivedPayload;

        provider.sendRequest(request, [&receivedPayload](const DownstreamResponse &response) noexcept {
            receivedPayload = response.payload;
        });

        // Response should be either positive (0x51) or negative (0x7F)
        CHECK(receivedPayload.size() > 0);
        CHECK((receivedPayload[0] == 0x51 || receivedPayload[0] == 0x7F));
    }

    TEST_CASE_FIXTURE(UdsMockProviderFixture, "multiple sequential requests are handled correctly") {
        ByteArray request1 = {0x3E, 0x00}; // TesterPresent
        ByteArray request2 = {0x22, 0xF1, 0x90}; // RDBI VIN
        ByteArray request3 = {0x3E, 0x00}; // TesterPresent again

        int callbackCount = 0;

        provider.sendRequest(request1, [&callbackCount](const DownstreamResponse &response) noexcept {
            callbackCount++;
            (void)response;
        });

        provider.sendRequest(request2, [&callbackCount](const DownstreamResponse &response) noexcept {
            callbackCount++;
            (void)response;
        });

        provider.sendRequest(request3, [&callbackCount](const DownstreamResponse &response) noexcept {
            callbackCount++;
            (void)response;
        });

        CHECK(callbackCount == 3);
    }

    TEST_CASE_FIXTURE(UdsMockProviderFixture, "response payload is not empty for valid TesterPresent request") {
        ByteArray request = {0x3E, 0x00};
        ByteArray receivedPayload;

        provider.sendRequest(request, [&receivedPayload](const DownstreamResponse &response) noexcept {
            receivedPayload = response.payload;
        });

        CHECK_FALSE(receivedPayload.empty());
    }

    TEST_CASE_FIXTURE(UdsMockProviderFixture, "sendRequest measures latency correctly") {
        ByteArray request = {0x3E, 0x00};
        std::chrono::milliseconds receivedLatency{-1};

        provider.sendRequest(request, [&receivedLatency](const DownstreamResponse &response) noexcept {
            receivedLatency = response.latency;
        });

        // Latency should be non-negative and reasonably small (synchronous processing)
        CHECK(receivedLatency.count() >= 0);
        CHECK(receivedLatency.count() < 1000); // Should complete in less than 1 second
    }

    TEST_CASE_FIXTURE(UdsMockProviderFixture, "sendRequest handles subfunction not supported") {
        // DiagnosticSessionControl with invalid subfunction
        ByteArray request = {0x10, 0xFF};
        ByteArray receivedPayload;

        provider.sendRequest(request, [&receivedPayload](const DownstreamResponse &response) noexcept {
            receivedPayload = response.payload;
        });

        // Expect negative response: 0x7F, 0x10, SubFunctionNotSupported (0x12)
        CHECK(receivedPayload.size() == 3);
        CHECK(receivedPayload[0] == 0x7F);
        CHECK(receivedPayload[1] == 0x10);
        CHECK(receivedPayload[2] == 0x12); // SubFunctionNotSupported
    }
}
