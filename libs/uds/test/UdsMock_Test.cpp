#include <doctest/doctest.h>
#include <memory>

#include "doctest_aux.h"
#include "uds/UdsMock.h"
#include "uds/IUdsModel.h"
#include "uds/UdsDefaultModel.h"
#include "uds/services/UdsReadDTCInformation.h"

using namespace doip;
using namespace doip::uds;

struct UdsTestModel : UdsDefaultModel {
    UdsTestModel() : UdsDefaultModel() {}

    virtual std::string_view getModelName() const override {
        return "UdsTestModel";
    }
};

TEST_SUITE("UdsMock") {
    struct UdsFixture {
        UdsMock udsMock{std::make_unique<UdsTestModel>()};

        UdsFixture() {
            // Setup code here if needed
        }

        ~UdsFixture() {
            // Cleanup code here if needed
        }
    };

    TEST_CASE_FIXTURE(UdsFixture, "UdsMock handles invalid service id") {
        ByteArray request = {0x00, 0x00}; // Invalid UDS service ID
        ByteArray response = udsMock.handleDiagnosticRequest(request);

        ByteArray expectedResponse = {0x7F, 0x00, 0x11};

        INFO(response);
        CHECK(response == expectedResponse);
    }


    TEST_CASE_FIXTURE(UdsFixture, "UdsMock default behavior returns ServiceNotSupported") {
        ByteArray request = {0x10, 0x01}; // Example UDS request (Diagnostic Session Control)
        ByteArray response = udsMock.handleDiagnosticRequest(request);

        // Expected negative response: 0x7F, 0x10, NRC for ServiceNotSupported (0x11)
        ByteArray expectedResponse = {0x7F, 0x10, 0x11};

        INFO(response);
        CHECK(response == expectedResponse);
    }

    TEST_CASE_FIXTURE(UdsFixture, "UdsMock custom handler returns positive response") {
        udsMock.registerService(uds::UdsService::DiagnosticSessionControl,
                                [](const ByteArray &request) {
                                    // Custom handler that returns positive response
                                    return ByteArray{sidResponseCode(request[0]), 1, 1, 2, 3, 4};
                                });

        ByteArray request = {0x10, 0x01}; // Example UDS request (Diagnostic Session Control)
        ByteArray response = udsMock.handleDiagnosticRequest(request);

        // Expected positive response: 0x50, 0x01, 0x01, 0x02, 0x03, 0x04
        ByteArray expectedResponse = {0x50, 0x01, 0x01, 0x02, 0x03, 0x04};

        INFO(response);
        CHECK(response == expectedResponse);
    }

    TEST_CASE_FIXTURE(UdsFixture, "UdsMock custom RDBI handler returns positive response") {
        udsMock.registerService(UdsService::ReadDataByIdentifier,
                                [](const ByteArray &request) {
                                    // Extract DID from request
                                    if (request.size() < 3) {
                                        return ByteArray{};
                                    }
                                    uint16_t did = request.readU16(1);
                                    // Custom handler that returns positive response with dummy data
                                    ByteArray responseData;
                                    responseData.writeU8(0x62); // Positive response SID for RDBI
                                    responseData.writeU16(did); // Echo back the DID
                                    responseData.writeU8(0x12); // Dummy data byte 1
                                    responseData.writeU8(0x34); // Dummy data byte 2
                                    return responseData;
                                });
        ByteArray request = {0x22, 0x01, 0x02}; // Example UDS request (Diagnostic Session Control)
        ByteArray response = udsMock.handleDiagnosticRequest(request);

        // Expected positive response: 0x50, 0x01
        ByteArray expectedResponse = {0x62, 0x01, 0x02, 0x12, 0x34};

        INFO(response);
        CHECK_BYTE_ARRAY_EQ(response, expectedResponse);

        // bad RDBI request -> invalid message format (0x13)
        request = {0x22, 0x01};
        response = udsMock.handleDiagnosticRequest(request);
        expectedResponse = {0x7f, 0x22, 0x13};

        INFO(response);
        CHECK_BYTE_ARRAY_EQ(response, expectedResponse);
    }

    TEST_CASE_FIXTURE(UdsFixture, "UdsMock read DTC information handler") {
        udsMock.registerDefaultServices();

        auto model = dynamic_cast<UdsTestModel*>(udsMock.getModel().get());
        REQUIRE(model != nullptr);

        ByteArray seedReq = {0x27, 0x01};
        ByteArray seedRsp = udsMock.handleDiagnosticRequest(seedReq);

        CHECK(seedRsp.size() == 6);
        CHECK(seedRsp[0] == 0x67);
        CHECK(seedRsp[1] == 0x01);

        // Extract seed
        ByteArray seed(seedRsp.data() + 2, seedRsp.size() - 2);
        uint32_t seedValue = seed.readU32(0);

        // Calculate expected key (Level 1 algorithm)
        uint32_t expectedKey = (seedValue ^ 0xA5A5A5A5) + 0x12345678;

        // Send key
        ByteArray keyReq = {0x27, 0x02};
        keyReq.writeU32(expectedKey);
        ByteArray keyRsp = udsMock.handleDiagnosticRequest(keyReq);

        // Check positive response
        CHECK(keyRsp.size() == 2);
        CHECK(keyRsp[0] == 0x67);
        CHECK(keyRsp[1] == 0x02);

        // Verify level is unlocked
        CHECK(model->isSecurityLevelUnlocked(1));

        // Add some DTCs to the model
        model->getDTCStore().addDTC(DiagnosticTroubleCode(0x123456, 0x01));
        model->getDTCStore().addDTC(DiagnosticTroubleCode(0x234567, 0x02));
        model->getDTCStore().addDTC(DiagnosticTroubleCode(0x123458, 0x01));

        // Create a Read DTC Information request for sub-function 0x01 (Report DTCs)
        ByteArray request = {0x19, 0x01, 0x01};
        ByteArray response = udsMock.handleDiagnosticRequest(request);

        // Expected positive response: 0x59, 0x01, followed by DTCs
        ByteArray expectedResponse = {0x59, 0x01, 0xff, DTC_FORMAT_IDENTIFIER, 0x00, 0x02};

        INFO(response);
        CHECK_BYTE_ARRAY_EQ(response, expectedResponse);
    }
}
