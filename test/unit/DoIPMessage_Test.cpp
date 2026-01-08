#include <cstdint>
#include <doctest/doctest.h>
#include <optional>

#include "DoIPMessage.h"
#include "util/ByteArray.h"

using namespace doip;

TEST_SUITE("DoIPMessage") {
    TEST_CASE("Message assembly") {
        DoIPMessage msg(DoIPPayloadType::AliveCheckRequest, {0x01, 0x02});
        ByteArray expected{
            0x03, 0xfc,             // protocol version + inv
            0x00, 0x07,             // payload type
            0x00, 0x00, 0x00, 0x02, // payload length
            0x01, 0x02              // payload
        };

        CHECK(msg.getPayloadSize() == 2);
        CHECK(msg.getMessageSize() == 10);
        CHECK(msg.getPayloadType() == DoIPPayloadType::AliveCheckRequest);

        const auto bytes = msg.asByteArray();

        for (size_t i = 0; i < bytes.size(); i++) {
            CHECK_MESSAGE(bytes.at(i) == expected.at(i), "Bytes to not match at pos ", i, ", got ", bytes.at(i), ", expected ", expected.at(i));
        }
    }

    TEST_CASE("Message factory - makeNegativeAckMessage") {
        DoIPMessage msg = message::makeNegativeAckMessage(DoIPNegativeAck::InvalidPayloadLength);
        ByteArray expected{
            0x03, 0xfc,             // protocol version + inv
            0x00, 0x00,             // payload type
            0x00, 0x00, 0x00, 0x01, // payload length
            0x04                    // payload
        };

        CHECK(msg.getPayloadSize() == 1);
        CHECK(msg.getMessageSize() == 9);
        CHECK(msg.getPayloadType() == DoIPPayloadType::NegativeAck);
    }

    TEST_CASE("Message factory - makeDiagnosticMessage") {
        DoIPMessage msg = message::makeDiagnosticMessage(DoIPAddress(0xcafe), DoIPAddress(0xbabe), {0xde, 0xad, 0xbe, 0xef});
        ByteArray expected{
            0x03, 0xfc,             // protocol version + inv
            0x80, 0x01,             // payload type
            0x00, 0x00, 0x00, 0x08, // payload length
            0xca, 0xfe,             // sa
            0xba, 0xbe,             // ta
            0xde, 0xad, 0xbe, 0xef  // payload
        };

        CHECK(msg.getPayloadSize() == 8);
        CHECK(msg.getMessageSize() == 16);
        CHECK(msg.getPayloadType() == DoIPPayloadType::DiagnosticMessage);

        const auto bytes = msg.asByteArray();
        for (size_t i = 0; i < bytes.size(); i++) {
            CHECK_MESSAGE(bytes.at(i) == expected.at(i), "Bytes to not match at pos ", i, ", got ", bytes.at(i), ", expected ", expected.at(i));
        }
    }

    TEST_CASE("Message factory - makeDiagnosticPositiveResponse") {
        DoIPMessage msg = message::makeDiagnosticPositiveResponse(DoIPAddress(0xcafe), DoIPAddress(0xbabe), {0xde, 0xad, 0xbe, 0xef});
        ByteArray expected{
            0x03, 0xfc,             // protocol version + inv
            0x80, 0x02,             // payload tyByteArray raw = message::makeAliveCheckResponse(DoIPAddress(0xa0b0));pe
            0x00, 0x00, 0x00, 0x09, // payload length
            0xca, 0xfe,             // sa
            0xba, 0xbe,             // ta
            0x00,                   // positive response
            0xde, 0xad, 0xbe, 0xef  // payload
        };

        CHECK(msg.getPayloadSize() == 9);
        CHECK(msg.getMessageSize() == 17);
        CHECK(msg.getPayloadType() == DoIPPayloadType::DiagnosticMessageAck);

        const auto bytes = msg.asByteArray();
        for (size_t i = 0; i < bytes.size(); i++) {
            CHECK_MESSAGE(bytes.at(i) == expected.at(i), "Bytes to not match at pos ", i, ", got ", bytes.at(i), ", expected ", expected.at(i));
        }
    }

    TEST_CASE("Message factory - makeDiagnosticNegativeResponse") {
        DoIPMessage msg = message::makeDiagnosticNegativeResponse(
            DoIPAddress(0xcafe),
            DoIPAddress(0xbabe),
            DoIPDiagnosticAck::TargetBusy,
            {0xde, 0xad, 0xbe, 0xef});

        ByteArray expected{
            0x03, 0xfc,             // protocol version + inv
            0x80, 0x03,             // payload type
            0x00, 0x00, 0x00, 0x09, // payload length
            0xca, 0xfe,             // sa
            0xba, 0xbe,             // ta
            0x09,                   // negative response code
            0xde, 0xad, 0xbe, 0xef  // payload
        };

        CHECK(msg.getPayloadSize() == 9);
        CHECK(msg.getMessageSize() == 17);
        CHECK(msg.getPayloadType() == DoIPPayloadType::DiagnosticMessageNegativeAck);

        const auto bytes = msg.asByteArray();
        for (size_t i = 0; i < bytes.size(); i++) {
            CHECK_MESSAGE(bytes.at(i) == expected.at(i), "Bytes to not match at pos ", i, ", got ", bytes.at(i), ", expected ", expected.at(i));
        }
    }

    TEST_CASE("Message factory - makeAliveCheckRequest") {
        DoIPMessage msg = message::makeAliveCheckRequest();

        CHECK(msg.getPayloadSize() == 0);
        CHECK(msg.getMessageSize() == 8);
        CHECK(msg.getPayloadType() == DoIPPayloadType::AliveCheckRequest);
    }

    TEST_CASE("Message factory - makeAliveCheckResponse") {
        DoIPMessage msg = message::makeAliveCheckResponse(DoIPAddress(0xa0b0));
        ByteArray expected{
            0x03,
            0xfc, // protocol version + inv
            0x00,
            0x08, // payload type
            0x00,
            0x00,
            0x00,
            0x02, // payload length
            0xa0,
            0xb0, // sa
        };

        CHECK(msg.getPayloadSize() == 2);
        CHECK(msg.getMessageSize() == 10);
        CHECK(msg.getPayloadType() == DoIPPayloadType::AliveCheckResponse);

        const auto bytes = msg.asByteArray();
        for (size_t i = 0; i < bytes.size(); i++) {
            CHECK_MESSAGE(bytes.at(i) == expected.at(i), "Bytes to not match at pos ", i, ", got ", bytes.at(i), ", expected ", expected.at(i));
        }
    }

    TEST_CASE("Message factory - makeVehicleIdentificationRequest") {
        DoIPMessage msg = message::makeVehicleIdentificationRequest();

        CHECK(msg.getPayloadSize() == 0);
        CHECK(msg.getMessageSize() == 8);
        CHECK(msg.getPayloadType() == DoIPPayloadType::VehicleIdentificationRequest);
    }

    TEST_CASE("Message factory - makeVehicleIdentificationResponse") {
        Vin vin = Vin("1HGCM82633A123456");
        DoIPAddress logicalAddress = DoIPAddress(1234);
        EntityId entityType = EntityId("EID123");
        GroupId groupId = GroupId("GID456");
        DoIPFurtherAction furtherAction = DoIPFurtherAction::RoutingActivationForCentralSecurity;
        DoIPMessage msg = message::makeVehicleIdentificationResponse(vin, logicalAddress, entityType, groupId, furtherAction);

        INFO(msg, "\n", logicalAddress);
        CHECK(msg.getPayloadSize() >= 31); // 17 + 2 + 6 + 6 + 1 (+ 1 optional byte for sync status)
        CHECK(msg.getMessageSize() >= 40);
        CHECK(msg.getPayloadType() == DoIPPayloadType::VehicleIdentificationResponse);

        // Example:
        // VIN: 45.58.41.4D.50.4C.45.53.45.52.56.45.52.30.30.30.30.00.
        // LA: 28.00.
        // EID: 00.00.00.00.00.00
        // GID: 00.00.00.00.00.00
        // FAR: 00
        // (sync status: optional)

        CHECK(msg.getVin().has_value());
        CHECK(msg.getVin().value().toString() == vin.toString());
        CHECK(msg.getLogicalAddress().has_value());
        CHECK(msg.getLogicalAddress().value() == logicalAddress);
        CHECK(msg.getEid().has_value());
        CHECK(msg.getEid().value().toString() == entityType.toString());
        CHECK(msg.getGid().has_value());
        CHECK(msg.getGid().value().toString() == groupId.toString());
        CHECK(msg.getFurtherActionRequest().has_value());
        CHECK(msg.getFurtherActionRequest().value() == furtherAction);
    }

    TEST_CASE("Init from raw bytes - invalid args") {
        const uint8_t short_msg[] = {PROTOCOL_VERSION, PROTOCOL_VERSION_INV, 0x80, 0x01};
        const uint8_t inv_protocol[] = {PROTOCOL_VERSION - 1, PROTOCOL_VERSION_INV + 1, 0x80, 0x01};
        const uint8_t inconsistent_protocol[] = {PROTOCOL_VERSION, PROTOCOL_VERSION_INV + 1, 0x80, 0x01};
        const uint8_t invalid_pl_type[] = {PROTOCOL_VERSION, PROTOCOL_VERSION_INV, 0xde, 0xad, 0x00, 0x02};
        const uint8_t invalid_pl_length1[] = {PROTOCOL_VERSION, PROTOCOL_VERSION_INV, 0x40, 0x01, 0x00, 0x02, 0x0};
        const uint8_t invalid_pl_length2[] = {PROTOCOL_VERSION, PROTOCOL_VERSION_INV, 0x40, 0x01, 0x00, 0x02, 0x0, 0x0, 0x0};

        CHECK(DoIPMessage::tryParse(nullptr, 12) == std::nullopt);                                          // null ptr
        CHECK(DoIPMessage::tryParse(short_msg, sizeof(short_msg)) == std::nullopt);                         // too short
        CHECK(DoIPMessage::tryParse(inv_protocol, sizeof(inv_protocol)) == std::nullopt);                   // wrong protocol
        CHECK(DoIPMessage::tryParse(inconsistent_protocol, sizeof(inconsistent_protocol)) == std::nullopt); // inconsistent protocol
        CHECK(DoIPMessage::tryParse(invalid_pl_type, sizeof(invalid_pl_type)) == std::nullopt);             // inconsistent payload type
        CHECK(DoIPMessage::tryParse(invalid_pl_length1, sizeof(invalid_pl_length1)) == std::nullopt);       // pl size > payload len
        CHECK(DoIPMessage::tryParse(invalid_pl_length2, sizeof(invalid_pl_length2)) == std::nullopt);       // pl size < payload len
    }

    TEST_CASE("Init from raw bytes - diagnostic message") {
        // Diag message with RDBI request
        const uint8_t example_diag[] = {PROTOCOL_VERSION, PROTOCOL_VERSION_INV, 0x80, 0x01, 0x00, 0x00, 0x00, 0x07, MIN_SOURCE_ADDRESS >> 8, MIN_SOURCE_ADDRESS & 0xFF, 0xca, 0xfe, 0x22, 0xFD, 0x10};
        auto opt_msg = DoIPMessage::tryParse(example_diag, sizeof(example_diag));

        REQUIRE_MESSAGE(opt_msg.has_value(), "No message was created");

        auto msg = opt_msg.value();
        CHECK(msg.getPayloadType() == DoIPPayloadType::DiagnosticMessage);
        CHECK(msg.getPayloadSize() == 7);

        ByteArray msg_conv = msg.asByteArray();
        CHECK(msg_conv.size() == 7 + DOIP_HEADER_SIZE);

        for (size_t i = 0; i < sizeof(example_diag); i++) {
            CHECK_MESSAGE(msg_conv.at(i) == example_diag[i], "Position ", i, " exp: ", example_diag[i], ", got: ", msg_conv.at(i));
        }

        auto optSa = msg.getSourceAddress();
        REQUIRE_MESSAGE(optSa.has_value(), "No source address extracted");
        CHECK(optSa.value() == MIN_SOURCE_ADDRESS);

        auto optTa = msg.getTargetAddress();
        REQUIRE_MESSAGE(optTa.has_value(), "No target address extracted");
        CHECK(optTa.value() == 0xcafe);

        auto optPayload = msg.getPayload();

        const size_t DIAG_MSG_OFFSET = 4; // after header + sa + ta

        CHECK(optPayload.second == 3 + DIAG_MSG_OFFSET); // size is the second element of the pair
        CHECK(optPayload.first[DIAG_MSG_OFFSET + 0] == 0x22);
        CHECK(optPayload.first[DIAG_MSG_OFFSET + 1] == 0xFD);
        CHECK(optPayload.first[DIAG_MSG_OFFSET + 2] == 0x10);
    }

    TEST_CASE("Message factory - Optional attributes: Activation request") {
        DoIPMessage msg = message::makeRoutingActivationRequest(0xe00);
        CHECK(msg.getVin() == std::nullopt);
        CHECK(msg.getSourceAddress() == 0xe00);
        CHECK(msg.getTargetAddress() == std::nullopt);
        CHECK(msg.getLogicalAddress() == std::nullopt);
        CHECK(msg.getGid() == std::nullopt);
        CHECK(msg.getEid() == std::nullopt);
        CHECK(msg.getFurtherActionRequest() == std::nullopt);
    }

    TEST_CASE("Message factory - Optional attributes: Keep alive") {
        DoIPMessage msg = message::makeAliveCheckResponse(0xe01);
        CHECK(msg.getVin() == std::nullopt);
        CHECK(msg.getSourceAddress() == 0xe01);
        CHECK(msg.getTargetAddress() == std::nullopt);
        CHECK(msg.getLogicalAddress() == std::nullopt);
        CHECK(msg.getGid() == std::nullopt);
        CHECK(msg.getEid() == std::nullopt);
        CHECK(msg.getFurtherActionRequest() == std::nullopt);
    }

    TEST_CASE("Message factory - Invalid protocol version") {
        ByteArray invProtocol1{
            0x01,
            0xfc, // mismatch
            0x00,
            0x08,
            0x00,
            0x00,
            0x00,
            0x02,
            0xa0,
            0xb0,
        };

        auto msg1 = DoIPMessage::tryParse(invProtocol1.data(), invProtocol1.size());
        CHECK(msg1 == std::nullopt);

        ByteArray invProtocol2{
            0x05,
            0xfa, // version 5 does not exist
            0x00,
            0x08, // payload type
            0x00,
            0x00,
            0x00,
            0x02, // payload length
            0xa0,
            0xb0, // sa
        };

        auto msg2 = DoIPMessage::tryParse(invProtocol2.data(), invProtocol2.size());
        CHECK(msg2 == std::nullopt);
    }

    TEST_CASE("Message factory - Invalid payload type") {
        for (uint16_t payloadType = 0; payloadType < 0xffff; payloadType++) {
            if (toPayloadType(payloadType) != std::nullopt)
                continue;

            ByteArray msg;
            msg.writeU8(PROTOCOL_VERSION);
            msg.writeU8(PROTOCOL_VERSION_INV);
            msg.writeU16(payloadType);
            msg.writeU32(0); // no payload

            auto msg1 = DoIPMessage::tryParse(msg.data(), msg.size());
            CHECK(msg1 == std::nullopt);
        }
    }
}
