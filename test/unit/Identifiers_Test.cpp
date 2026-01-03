#include <doctest/doctest.h>
#include "doctest_aux.h"


#include "DoIPIdentifiers.h"

#include <sstream>

using namespace doip;

TEST_SUITE("DoIP Identifiers") {

    TEST_CASE("EntityId - Entity Identifier (6 bytes)") {
        SUBCASE("Default constructor") {
            EntityId eid;
            CHECK(eid.isEmpty());
            CHECK(eid.size() == 6);
            CHECK(eid.toString().empty());
        }

        SUBCASE("Static Zero instance") {
            CHECK(EntityId::Zero.isEmpty());
            CHECK(EntityId::Zero.size() == 6);
        }

        SUBCASE("Construction from string - exact length") {
            EntityId eid("ABC123");
            CHECK(eid.toString() == "ABC123");
            CHECK(eid.size() == 6);
            CHECK_FALSE(eid.isEmpty());
        }

        SUBCASE("Construction from string - shorter") {
            EntityId eid("EID");
            CHECK(eid.toString() == "EID");
            CHECK(eid.size() == 6);
            CHECK(eid[0] == 'E');
            CHECK(eid[2] == 'D');
            CHECK(eid[3] == 0);
            CHECK(eid[5] == 0);
        }

        SUBCASE("Construction from string - longer") {
            EntityId eid("TOOLONGEID");
            CHECK(eid.toString() == "TOOLON");
            CHECK(eid.size() == 6);
        }

        SUBCASE("Construction from byte array") {
            const uint8_t bytes[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
            EntityId eid(bytes, sizeof(bytes));
            CHECK(eid.size() == 6);
            CHECK(eid[0] == 0x01);
            CHECK(eid[5] == 0x06);
            CHECK_FALSE(eid.isEmpty());
        }

        SUBCASE("Construction from ByteArray") {
            ByteArray bytes = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
            EntityId eid(bytes);
            CHECK(eid.size() == 6);
            CHECK(eid[0] == 0xAA);
            CHECK(eid[5] == 0xFF);
        }

        SUBCASE("Equality and comparison") {
            EntityId eid1("EID001");
            EntityId eid2("EID001");
            EntityId eid3("EID002");

            CHECK(eid1 == eid2);
            CHECK(eid1 != eid3);
            CHECK(eid2 != eid3);
        }

        SUBCASE("asByteArray method") {
            EntityId eid("TEST12");
            ByteArrayRef result = eid.asByteArray();
            CHECK(result.second == 6);
            CHECK(result.first[0] == 'T');
            CHECK(result.first[5] == '2');
        }
    }

    TEST_CASE("GroupId - Group Identifier (6 bytes)") {
        SUBCASE("Default constructor") {
            GroupId gid;
            CHECK(gid.isEmpty());
            CHECK(gid.size() == 6);
            CHECK(gid.toString().empty());
        }

        SUBCASE("Static Zero instance") {
            CHECK(GroupId::Zero.isEmpty());
            CHECK(GroupId::Zero.size() == 6);
        }

        SUBCASE("Construction from string - exact length") {
            GroupId gid("GRP001");
            CHECK(gid.toString() == "GRP001");
            CHECK(gid.size() == 6);
            CHECK_FALSE(gid.isEmpty());
        }

        SUBCASE("Construction from string - shorter") {
            GroupId gid("GID");
            CHECK(gid.toString() == "GID");
            CHECK(gid.size() == 6);
            CHECK(gid[0] == 'G');
            CHECK(gid[2] == 'D');
            CHECK(gid[3] == 0);
            CHECK(gid[5] == 0);
        }

        SUBCASE("Construction from string - longer") {
            GroupId gid("TOOLONGGID");
            CHECK(gid.toString() == "TOOLON");
            CHECK(gid.size() == 6);
        }

        SUBCASE("Construction from uint32_t - longer") {
            uint32_t long_value = 0x544F4F4C; // ASCII for "TOOL"
            GroupId gid(long_value);
            CHECK(gid.toString() == "TOOL");
            CHECK(gid.size() == 6);
        }


        SUBCASE("Construction from uint64_t - longer") {
            uint64_t long_value = 0x544F4F4C4F4E47; // ASCII for "TOOLONG"
            GroupId gid(long_value);
            CHECK(gid.toString() == "OOLONG"); //
            CHECK(gid.size() == 6);
        }

        SUBCASE("Construction from byte array") {
            const uint8_t bytes[] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60};
            GroupId gid(bytes, sizeof(bytes));
            CHECK(gid.size() == 6);
            CHECK(gid[0] == 0x10);
            CHECK(gid[5] == 0x60);
            CHECK_FALSE(gid.isEmpty());
        }

        SUBCASE("Construction from ByteArray") {
            ByteArray bytes = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
            GroupId gid(bytes);
            CHECK(gid.size() == 6);
            CHECK(gid[0] == 0x11);
            CHECK(gid[5] == 0x66);
        }

        SUBCASE("Equality and comparison") {
            GroupId gid1("GROUP1");
            GroupId gid2("GROUP1");
            GroupId gid3("GROUP2");

            CHECK(gid1 == gid2);
            CHECK(gid1 != gid3);
            CHECK(gid2 != gid3);
        }

        SUBCASE("asByteArray method") {
            GroupId gid("MYGRP1");
            ByteArrayRef result = gid.asByteArray();
            CHECK(result.second == 6);
            CHECK(result.first[0] == 'M');
            CHECK(result.first[5] == '1');
        }
    }

    TEST_CASE("Different identifier types are independent") {
        // Even though EID and GID have the same length, they're different types
        EntityId eid("ABC123");
        GroupId gid("ABC123");

        // They should have the same content but be different types
        CHECK(eid.toString() == gid.toString());
        CHECK(eid.size() == gid.size());

        // Verify they're truly independent instances
        EntityId eid2(eid);
        GroupId gid2(gid);
        CHECK(eid == eid2);
        CHECK(gid == gid2);
    }
}
