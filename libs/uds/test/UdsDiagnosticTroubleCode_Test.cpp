#include <doctest/doctest.h>

#include "uds/UdsDiagnosticTroubleCode.h"

using namespace doip::uds;

TEST_SUITE("DiagnosticTroubleCode") {
    TEST_CASE("Default constructor creates invalid DTC with code 0x000000") {
        DiagnosticTroubleCode dtc;
        CHECK(dtc.getCode() == 0x000000);
        CHECK(dtc.getStatusBits() == 0x00);
        CHECK(!dtc.isValid());
    }

    TEST_CASE("Constructor from code and status bits") {
        DiagnosticTroubleCode dtc(0x123456, 0xAB);
        CHECK(dtc.getCode() == 0x123456);
        CHECK(dtc.getStatusBits() == 0xAB);
        CHECK(dtc.isValid());
    }

    TEST_CASE("Constructor from individual bytes") {
        DiagnosticTroubleCode dtc(0x12, 0x34, 0x56, 0xCD);
        CHECK(dtc.getHighByte() == 0x12);
        CHECK(dtc.getMiddleByte() == 0x34);
        CHECK(dtc.getLowByte() == 0x56);
        CHECK(dtc.getStatusBits() == 0xCD);
        CHECK(dtc.getCode() == 0x123456);
    }

    TEST_CASE("Constructor only stores 24-bit code (masks upper 8 bits)") {
        DiagnosticTroubleCode dtc(0xFF123456, 0x00);
        CHECK(dtc.getCode() == 0x123456);  // Upper bits masked off
    }

    TEST_CASE("Byte getters extract correct bytes from code") {
        DiagnosticTroubleCode dtc(0xAABBCC, 0x00);
        CHECK(dtc.getHighByte() == 0xAA);
        CHECK(dtc.getMiddleByte() == 0xBB);
        CHECK(dtc.getLowByte() == 0xCC);
    }

    TEST_CASE("Status bit operations - set and check") {
        DiagnosticTroubleCode dtc;

        // Initially no status bits set
        CHECK(!dtc.hasStatusBit(DiagnosticTroubleCode::STATUS_TEST_FAILED));

        // Set status bit
        dtc.setStatusBit(DiagnosticTroubleCode::STATUS_TEST_FAILED);
        CHECK(dtc.hasStatusBit(DiagnosticTroubleCode::STATUS_TEST_FAILED));

        // Set another bit
        dtc.setStatusBit(DiagnosticTroubleCode::STATUS_CONFIRMED_DTC);
        CHECK(dtc.hasStatusBit(DiagnosticTroubleCode::STATUS_CONFIRMED_DTC));
        CHECK(dtc.hasStatusBit(DiagnosticTroubleCode::STATUS_TEST_FAILED));  // Still set
    }

    TEST_CASE("Status bit operations - clear") {
        DiagnosticTroubleCode dtc(0x000000, 0xFF);  // All bits set

        CHECK(dtc.hasStatusBit(DiagnosticTroubleCode::STATUS_TEST_FAILED));
        dtc.clearStatusBit(DiagnosticTroubleCode::STATUS_TEST_FAILED);
        CHECK(!dtc.hasStatusBit(DiagnosticTroubleCode::STATUS_TEST_FAILED));

        // Other bits should still be set
        CHECK(dtc.hasStatusBit(DiagnosticTroubleCode::STATUS_TEST_FAILED_THIS_CYCLE));
    }

    TEST_CASE("setStatusBits replaces all bits") {
        DiagnosticTroubleCode dtc(0x000000, 0xFF);
        dtc.setStatusBits(0x03);
        CHECK(dtc.getStatusBits() == 0x03);
        CHECK(dtc.hasStatusBit(DiagnosticTroubleCode::STATUS_TEST_FAILED));
        CHECK(dtc.hasStatusBit(DiagnosticTroubleCode::STATUS_TEST_FAILED_THIS_CYCLE));
        CHECK(!dtc.hasStatusBit(DiagnosticTroubleCode::STATUS_PENDING_DTC));
    }

    TEST_CASE("isConfirmed checks both testFailed and confirmedDTC bits") {
        DiagnosticTroubleCode dtc1(0x000000,
                                   DiagnosticTroubleCode::STATUS_TEST_FAILED |
                                       DiagnosticTroubleCode::STATUS_CONFIRMED_DTC);
        CHECK(dtc1.isConfirmed());

        DiagnosticTroubleCode dtc2(0x000000, DiagnosticTroubleCode::STATUS_TEST_FAILED);
        CHECK(!dtc2.isConfirmed());  // No confirmedDTC bit

        DiagnosticTroubleCode dtc3(0x000000, DiagnosticTroubleCode::STATUS_CONFIRMED_DTC);
        CHECK(!dtc3.isConfirmed());  // No testFailed bit
    }

    TEST_CASE("isPending checks pendingDTC without confirmedDTC") {
        DiagnosticTroubleCode dtc1(0x000000, DiagnosticTroubleCode::STATUS_PENDING_DTC);
        CHECK(dtc1.isPending());

        DiagnosticTroubleCode dtc2(0x000000,
                                   DiagnosticTroubleCode::STATUS_PENDING_DTC |
                                       DiagnosticTroubleCode::STATUS_CONFIRMED_DTC);
        CHECK(!dtc2.isPending());  // Also has confirmedDTC

        DiagnosticTroubleCode dtc3(0x000000, DiagnosticTroubleCode::STATUS_CONFIRMED_DTC);
        CHECK(!dtc3.isPending());  // No pendingDTC bit
    }

    TEST_CASE("hasActiveFailure checks testFailed bit") {
        DiagnosticTroubleCode dtc1(0x000000, DiagnosticTroubleCode::STATUS_TEST_FAILED);
        CHECK(dtc1.hasActiveFailure());

        DiagnosticTroubleCode dtc2(0x000000, 0x00);
        CHECK(!dtc2.hasActiveFailure());

        DiagnosticTroubleCode dtc3(0x000000, DiagnosticTroubleCode::STATUS_CONFIRMED_DTC);
        CHECK(!dtc3.hasActiveFailure());  // Confirmed without active failure
    }

    TEST_CASE("getSeverity extracts severity from bits 6-7 of high byte") {
        // Severity bits in high byte: bits 6-7
        // 00 = Informational, 01 = Warning, 10 = Error, 11 = Critical

        DiagnosticTroubleCode dtc1(0x00BBCC, 0x00);  // 00xxxxxx = Informational
        CHECK(dtc1.getSeverity() == DiagnosticTroubleCode::Severity::Informational);

        DiagnosticTroubleCode dtc2(0x40BBCC, 0x00);  // 01xxxxxx = Warning
        CHECK(dtc2.getSeverity() == DiagnosticTroubleCode::Severity::Warning);

        DiagnosticTroubleCode dtc3(0x80BBCC, 0x00);  // 10xxxxxx = Error
        CHECK(dtc3.getSeverity() == DiagnosticTroubleCode::Severity::Error);

        DiagnosticTroubleCode dtc4(0xC0BBCC, 0x00);  // 11xxxxxx = Critical
        CHECK(dtc4.getSeverity() == DiagnosticTroubleCode::Severity::Critical);
    }

    TEST_CASE("serialize produces 4-byte array") {
        DiagnosticTroubleCode dtc(0x12, 0x34, 0x56, 0xAB);
        auto serialized = dtc.serialize();

        CHECK(serialized.size() == 4);
        CHECK(serialized[0] == 0x12);
        CHECK(serialized[1] == 0x34);
        CHECK(serialized[2] == 0x56);
        CHECK(serialized[3] == 0xAB);
    }

    TEST_CASE("deserialize from array") {
        std::array<uint8_t, 4> data = {0xAA, 0xBB, 0xCC, 0xDD};
        DiagnosticTroubleCode dtc;

        bool result = dtc.deserialize(data);
        CHECK(result);
        CHECK(dtc.getCode() == 0xAABBCC);
        CHECK(dtc.getStatusBits() == 0xDD);
    }

    TEST_CASE("deserialize and serialize round-trip") {
        DiagnosticTroubleCode dtc1(0x123456, 0xAB);
        auto serialized = dtc1.serialize();

        DiagnosticTroubleCode dtc2;
        dtc2.deserialize(serialized);

        CHECK(dtc2.getCode() == dtc1.getCode());
        CHECK(dtc2.getStatusBits() == dtc1.getStatusBits());
        CHECK(dtc1 == dtc2);
    }

    TEST_CASE("deserialize from vector") {
        std::vector<uint8_t> data = {0xFF, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
        DiagnosticTroubleCode dtc;

        bool result = dtc.deserialize(data, 1);  // Start at offset 1
        CHECK(result);
        CHECK(dtc.getCode() == 0xAABBCC);
        CHECK(dtc.getStatusBits() == 0xDD);
    }

    TEST_CASE("deserialize from vector fails with insufficient data") {
        std::vector<uint8_t> data = {0xAA, 0xBB, 0xCC};  // Only 3 bytes
        DiagnosticTroubleCode dtc;

        bool result = dtc.deserialize(data, 0);
        CHECK(!result);
    }

    TEST_CASE("deserialize from vector fails with bad offset") {
        std::vector<uint8_t> data = {0xAA, 0xBB, 0xCC, 0xDD};
        DiagnosticTroubleCode dtc;

        bool result = dtc.deserialize(data, 1);  // Would need 5 bytes total
        CHECK(!result);
    }

    TEST_CASE("Equality comparison") {
        DiagnosticTroubleCode dtc1(0x123456, 0xAB);
        DiagnosticTroubleCode dtc2(0x123456, 0xAB);
        DiagnosticTroubleCode dtc3(0x123456, 0xCD);
        DiagnosticTroubleCode dtc4(0x654321, 0xAB);

        CHECK(dtc1 == dtc2);
        CHECK(!(dtc1 == dtc3));  // Different status
        CHECK(!(dtc1 == dtc4));  // Different code
    }

    TEST_CASE("Inequality comparison") {
        DiagnosticTroubleCode dtc1(0x123456, 0xAB);
        DiagnosticTroubleCode dtc2(0x123456, 0xAB);
        DiagnosticTroubleCode dtc3(0x654321, 0xCD);

        CHECK(!(dtc1 != dtc2));
        CHECK(dtc1 != dtc3);
    }

    TEST_CASE("Less-than comparison sorts by DTC code") {
        DiagnosticTroubleCode dtc1(0x100000, 0xAA);
        DiagnosticTroubleCode dtc2(0x200000, 0xBB);

        CHECK(dtc1 < dtc2);
        CHECK(!(dtc2 < dtc1));
        CHECK(!(dtc1 < dtc1));
    }

    TEST_CASE("getStatusDescription returns human-readable string") {
        DiagnosticTroubleCode dtc1(0x000000, 0x00);
        CHECK(dtc1.getStatusDescription() == "");

        DiagnosticTroubleCode dtc2(0x000000,
                                   DiagnosticTroubleCode::STATUS_TEST_FAILED |
                                       DiagnosticTroubleCode::STATUS_CONFIRMED_DTC);
        std::string desc = dtc2.getStatusDescription();
        CHECK(desc.find("testFailed") != std::string::npos);
        CHECK(desc.find("confirmedDTC") != std::string::npos);

        DiagnosticTroubleCode dtc3(0x000000, 0xFF);  // All bits set
        std::string desc_all = dtc3.getStatusDescription();
        CHECK(desc_all.find("testFailed") != std::string::npos);
        CHECK(desc_all.find("warningIndicatorRequested") != std::string::npos);
    }
}

TEST_SUITE("DiagnosticTroubleCodeStore") {
    TEST_CASE("Default store is empty") {
        DiagnosticTroubleCodeStore store;
        CHECK(store.count() == 0);
        CHECK(!store.hasDTC(0x123456));
    }

    TEST_CASE("addDTC adds a new DTC") {
        DiagnosticTroubleCodeStore store;
        DiagnosticTroubleCode dtc(0x123456, 0xAB);

        bool result = store.addDTC(dtc);
        CHECK(result);
        CHECK(store.count() == 1);
        CHECK(store.hasDTC(0x123456));
    }

    TEST_CASE("addDTC returns false for duplicate") {
        DiagnosticTroubleCodeStore store;
        DiagnosticTroubleCode dtc(0x123456, 0xAB);

        CHECK(store.addDTC(dtc));
        CHECK(!store.addDTC(dtc));  // Duplicate
        CHECK(store.count() == 1);
    }

    TEST_CASE("removeDTC removes an existing DTC") {
        DiagnosticTroubleCodeStore store;
        DiagnosticTroubleCode dtc(0x123456, 0xAB);
        store.addDTC(dtc);

        bool result = store.removeDTC(0x123456);
        CHECK(result);
        CHECK(store.count() == 0);
        CHECK(!store.hasDTC(0x123456));
    }

    TEST_CASE("removeDTC returns false for non-existent DTC") {
        DiagnosticTroubleCodeStore store;
        bool result = store.removeDTC(0x123456);
        CHECK(!result);
    }

    TEST_CASE("clearAll removes all DTCs") {
        DiagnosticTroubleCodeStore store;
        store.addDTC(DiagnosticTroubleCode(0x111111, 0xAA));
        store.addDTC(DiagnosticTroubleCode(0x222222, 0xBB));
        store.addDTC(DiagnosticTroubleCode(0x333333, 0xCC));

        CHECK(store.count() == 3);
        store.clearAll();
        CHECK(store.count() == 0);
    }

    TEST_CASE("findDTC returns pointer to DTC") {
        DiagnosticTroubleCodeStore store;
        DiagnosticTroubleCode dtc(0x123456, 0xAB);
        store.addDTC(dtc);

        const DiagnosticTroubleCode *found = store.findDTC(0x123456);
        CHECK(found != nullptr);
        CHECK(found->getCode() == 0x123456);
        CHECK(found->getStatusBits() == 0xAB);
    }

    TEST_CASE("findDTC returns nullptr for non-existent DTC") {
        DiagnosticTroubleCodeStore store;
        CHECK(store.findDTC(0x123456) == nullptr);
    }

    TEST_CASE("getConfirmedDTCs returns only confirmed DTCs") {
        DiagnosticTroubleCodeStore store;
        store.addDTC(DiagnosticTroubleCode(0x111111,
                                           DiagnosticTroubleCode::STATUS_TEST_FAILED |
                                               DiagnosticTroubleCode::STATUS_CONFIRMED_DTC));
        store.addDTC(DiagnosticTroubleCode(0x222222, DiagnosticTroubleCode::STATUS_PENDING_DTC));
        store.addDTC(DiagnosticTroubleCode(0x333333,
                                           DiagnosticTroubleCode::STATUS_TEST_FAILED |
                                               DiagnosticTroubleCode::STATUS_CONFIRMED_DTC));

        auto confirmed = store.getConfirmedDTCs();
        CHECK(confirmed.size() == 2);
        CHECK(confirmed[0].getCode() == 0x111111);
        CHECK(confirmed[1].getCode() == 0x333333);
    }

    TEST_CASE("getPendingDTCs returns only pending DTCs") {
        DiagnosticTroubleCodeStore store;
        store.addDTC(DiagnosticTroubleCode(0x111111, DiagnosticTroubleCode::STATUS_PENDING_DTC));
        store.addDTC(DiagnosticTroubleCode(0x222222, DiagnosticTroubleCode::STATUS_TEST_FAILED));
        store.addDTC(DiagnosticTroubleCode(0x333333, DiagnosticTroubleCode::STATUS_PENDING_DTC));

        auto pending = store.getPendingDTCs();
        CHECK(pending.size() == 2);
        CHECK(pending[0].getCode() == 0x111111);
        CHECK(pending[1].getCode() == 0x333333);
    }

    TEST_CASE("getActiveDTCs returns DTCs with testFailed bit") {
        DiagnosticTroubleCodeStore store;
        store.addDTC(
            DiagnosticTroubleCode(0x111111, DiagnosticTroubleCode::STATUS_TEST_FAILED));
        store.addDTC(DiagnosticTroubleCode(0x222222, DiagnosticTroubleCode::STATUS_PENDING_DTC));
        store.addDTC(
            DiagnosticTroubleCode(0x333333, DiagnosticTroubleCode::STATUS_TEST_FAILED));

        auto active = store.getActiveDTCs();
        CHECK(active.size() == 2);
        CHECK(active[0].getCode() == 0x111111);
        CHECK(active[1].getCode() == 0x333333);
    }

    TEST_CASE("serialize produces byte stream of all DTCs") {
        DiagnosticTroubleCodeStore store;
        store.addDTC(DiagnosticTroubleCode(0x111111, 0xAA));
        store.addDTC(DiagnosticTroubleCode(0x222222, 0xBB));

        auto serialized = store.serialize();
        CHECK(serialized.size() == 8);  // 2 DTCs × 4 bytes each
    }

    TEST_CASE("getDTCAt returns DTC by index") {
        DiagnosticTroubleCodeStore store;
        store.addDTC(DiagnosticTroubleCode(0x111111, 0xAA));
        store.addDTC(DiagnosticTroubleCode(0x222222, 0xBB));

        const DiagnosticTroubleCode *dtc0 = store.getDTCAt(0);
        CHECK(dtc0 != nullptr);
        CHECK(dtc0->getCode() == 0x111111);

        const DiagnosticTroubleCode *dtc1 = store.getDTCAt(1);
        CHECK(dtc1 != nullptr);
        CHECK(dtc1->getCode() == 0x222222);

        const DiagnosticTroubleCode *dtc_invalid = store.getDTCAt(999);
        CHECK(dtc_invalid == nullptr);
    }

    TEST_CASE("getAllDTCs returns const reference to all DTCs") {
        DiagnosticTroubleCodeStore store;
        store.addDTC(DiagnosticTroubleCode(0x111111, 0xAA));
        store.addDTC(DiagnosticTroubleCode(0x222222, 0xBB));

        const auto &all = store.getAllDTCs();
        CHECK(all.size() == 2);
        CHECK(all[0].getCode() == 0x111111);
        CHECK(all[1].getCode() == 0x222222);
    }

    TEST_CASE("DTCs are maintained in sorted order") {
        DiagnosticTroubleCodeStore store;
        store.addDTC(DiagnosticTroubleCode(0x333333, 0xCC));
        store.addDTC(DiagnosticTroubleCode(0x111111, 0xAA));
        store.addDTC(DiagnosticTroubleCode(0x222222, 0xBB));

        const auto &all = store.getAllDTCs();
        CHECK(all[0].getCode() == 0x111111);
        CHECK(all[1].getCode() == 0x222222);
        CHECK(all[2].getCode() == 0x333333);
    }
}
