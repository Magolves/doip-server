#include <doctest/doctest.h>
#include "uds/UdsSecureModel.h"
#include "uds/services/UdsSecurityAccess.h"
#include "uds/UdsServices.h"

using namespace doip;
using namespace doip::uds;

TEST_SUITE("SecurityAccess") {

    struct SecurityAccessFixture {
        UniqueUdsModelPtr model;
        SecurityAccessHandler handler;

        SecurityAccessFixture() : model(std::make_unique<UdsSecureModel>()) {}
    };

    TEST_CASE_FIXTURE(SecurityAccessFixture, "RequestSeed Level 1 - Programming") {
        // Request seed for programming session (level 1)
        ByteArray request = {0x27, 0x01};
        ByteArray response = handler.handle(request, model);

        // Check positive response: 0x67, 0x01, <4 seed bytes>
        CHECK(response.size() == 6);
        CHECK(response[0] == 0x67); // Positive response SID
        CHECK(response[1] == 0x01); // Security level
        // Seed should be non-zero
        bool allZero = true;
        for (size_t i = 2; i < response.size(); i++) {
            if (response[i] != 0x00) {
                allZero = false;
                break;
            }
        }
        CHECK_FALSE(allZero);
    }

    TEST_CASE_FIXTURE(SecurityAccessFixture, "RequestSeed when already unlocked returns zeros") {
        // First unlock the level
        ByteArray seedReq = {0x27, 0x01};
        ByteArray seedRsp = handler.handle(seedReq, model);

        // Extract seed and calculate key
        ByteArray seed(seedRsp.data() + 2, seedRsp.size() - 2);
        uint32_t seedValue = seed.readU32BE(0);
        uint32_t keyValue = (seedValue ^ 0xA5A5A5A5) + 0x12345678;

        ByteArray keyReq = {0x27, 0x02};
        keyReq.writeU32BE(keyValue);
        ByteArray keyRsp = handler.handle(keyReq, model);
        CHECK(keyRsp[0] == 0x67); // Unlocked

        // Now request seed again - should return all zeros
        ByteArray seedReq2 = {0x27, 0x01};
        ByteArray seedRsp2 = handler.handle(seedReq2, model);

        CHECK(seedRsp2.size() == 6);
        CHECK(seedRsp2[0] == 0x67);
        CHECK(seedRsp2[2] == 0x00);
        CHECK(seedRsp2[3] == 0x00);
        CHECK(seedRsp2[4] == 0x00);
        CHECK(seedRsp2[5] == 0x00);
    }

    TEST_CASE_FIXTURE(SecurityAccessFixture, "SendKey with valid key unlocks level") {
        // Request seed
        ByteArray seedReq = {0x27, 0x01};
        ByteArray seedRsp = handler.handle(seedReq, model);

        // Extract seed
        ByteArray seed(seedRsp.data() + 2, seedRsp.size() - 2);
        uint32_t seedValue = seed.readU32BE(0);

        // Calculate expected key (Level 1 algorithm)
        uint32_t expectedKey = (seedValue ^ 0xA5A5A5A5) + 0x12345678;

        // Send key
        ByteArray keyReq = {0x27, 0x02};
        keyReq.writeU32BE(expectedKey);
        ByteArray keyRsp = handler.handle(keyReq, model);

        // Check positive response
        CHECK(keyRsp.size() == 2);
        CHECK(keyRsp[0] == 0x67);
        CHECK(keyRsp[1] == 0x02);

        // Verify level is unlocked
        CHECK(model->isSecurityLevelUnlocked(1));
    }

    TEST_CASE_FIXTURE(SecurityAccessFixture, "SendKey with invalid key returns InvalidKey") {
        // Request seed
        ByteArray seedReq = {0x27, 0x01};
        handler.handle(seedReq, model);

        // Send wrong key
        ByteArray keyReq = {0x27, 0x02, 0x12, 0x34, 0x56, 0x78};
        ByteArray keyRsp = handler.handle(keyReq, model);

        // Check negative response: 0x7F, 0x27, 0x35 (InvalidKey)
        CHECK(keyRsp.size() == 3);
        CHECK(keyRsp[0] == 0x7F);
        CHECK(keyRsp[1] == 0x27);
        CHECK(keyRsp[2] == static_cast<uint8_t>(UdsResponseCode::InvalidKey));
    }

    TEST_CASE_FIXTURE(SecurityAccessFixture, "Exceeding attempts returns ExceededNumberOfAttempts") {
        for (int attempt = 0; attempt < 3; attempt++) {
            // Request seed
            ByteArray seedReq = {0x27, 0x01};
            handler.handle(seedReq, model);

            // Send wrong key
            ByteArray keyReq = {0x27, 0x02, 0x00, 0x00, 0x00, 0x00};
            ByteArray keyRsp = handler.handle(keyReq, model);

            CHECK(keyRsp[0] == 0x7F);
        }

        // Fourth attempt should fail immediately
        ByteArray seedReq = {0x27, 0x01};
        ByteArray seedRsp = handler.handle(seedReq, model);

        CHECK(seedRsp.size() == 3);
        CHECK(seedRsp[0] == 0x7F);
        CHECK(seedRsp[1] == 0x27);
        CHECK(seedRsp[2] == static_cast<uint8_t>(UdsResponseCode::ExceededNumberOfAttempts));
    }

    TEST_CASE_FIXTURE(SecurityAccessFixture, "SendKey without RequestSeed returns RequestSequenceError") {
        // Try to send key without requesting seed first
        ByteArray keyReq = {0x27, 0x02, 0x12, 0x34, 0x56, 0x78};
        ByteArray keyRsp = handler.handle(keyReq, model);

        CHECK(keyRsp.size() == 3);
        CHECK(keyRsp[0] == 0x7F);
        CHECK(keyRsp[1] == 0x27);
        CHECK(keyRsp[2] == static_cast<uint8_t>(UdsResponseCode::RequestSequenceError));
    }

    TEST_CASE_FIXTURE(SecurityAccessFixture, "Session change locks security levels") {
        // Unlock level 1
        ByteArray seedReq = {0x27, 0x01};
        ByteArray seedRsp = handler.handle(seedReq, model);

        ByteArray seed(seedRsp.data() + 2, seedRsp.size() - 2);
        uint32_t seedValue = seed.readU32BE(0);
        uint32_t keyValue = (seedValue ^ 0xA5A5A5A5) + 0x12345678;

        ByteArray keyReq = {0x27, 0x02};
        keyReq.writeU32BE(keyValue);
        handler.handle(keyReq, model);

        CHECK(model->isSecurityLevelUnlocked(1));

        // Change session
        model->setCurrentSession(DiagnosticSessionControlType::ProgrammingSession);

        // Level should be locked again
        CHECK_FALSE(model->isSecurityLevelUnlocked(1));
    }

    TEST_CASE_FIXTURE(SecurityAccessFixture, "Invalid sub-function returns SubFunctionNotSupported") {
        // Sub-function 0x00 is reserved
        ByteArray request = {0x27, 0x00};
        ByteArray response = handler.handle(request, model);

        CHECK(response[0] == 0x7F);
        CHECK(response[2] == static_cast<uint8_t>(UdsResponseCode::SubFunctionNotSupported));

        // Sub-function > 0x7F is invalid
        ByteArray request2 = {0x27, 0x80};
        ByteArray response2 = handler.handle(request2, model);

        CHECK(response2[0] == 0x7F);
        CHECK(response2[2] == static_cast<uint8_t>(UdsResponseCode::SubFunctionNotSupported));
    }

    TEST_CASE_FIXTURE(SecurityAccessFixture, "Level 3 Extended Diagnostic - different algorithm") {
        // Request seed for level 3
        ByteArray seedReq = {0x27, 0x03};
        ByteArray seedRsp = handler.handle(seedReq, model);

        CHECK(seedRsp.size() == 6);
        CHECK(seedRsp[0] == 0x67);

        // Extract seed
        ByteArray seed(seedRsp.data() + 2, seedRsp.size() - 2);
        uint32_t seedValue = seed.readU32BE(0);

        // Calculate key using Level 3 algorithm (rotate left 5 bits + XOR mask)
        uint32_t rotated = (seedValue << 5) | (seedValue >> 27);
        uint32_t expectedKey = rotated ^ 0x5A5A5A5A;

        // Send key
        ByteArray keyReq = {0x27, 0x04};
        keyReq.writeU32BE(expectedKey);
        ByteArray keyRsp = handler.handle(keyReq, model);

        CHECK(keyRsp[0] == 0x67);
        CHECK(model->isSecurityLevelUnlocked(3));
    }
}
