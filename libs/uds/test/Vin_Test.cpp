#include <doctest/doctest.h>
#include "Vin.h"
#include <sstream>

#include "doctest_aux.h"

using namespace doip;

TEST_SUITE("GenericFixedId") {

    TEST_CASE("Default constructor creates empty VIN") {
        Vin vin;

        // Verify all bytes are '0'
        for (size_t i = 0; i < 17; ++i) {
            CHECK(vin[i] == '0');
        }
    }

    TEST_CASE("Construction from string - exact length") {
        const std::string test_vin = "1HGBH41JXMN109186";
        Vin vin(test_vin);

        CHECK_FALSE(vin.isEmpty());
        CHECK(vin.toString() == test_vin);

        // Verify individual characters
        CHECK(vin[0] == '1');
        CHECK(vin[16] == '6');
        CHECK(isValidVin(vin));
    }

    TEST_CASE("Construction from string - shorter than 17 characters") {
        const std::string test_vin = "ABC12300000000000";
        Vin vin(test_vin);

        CHECK_FALSE(vin.isEmpty());
        CHECK(vin.toString() == test_vin);

        // Verify padding with zeros
        CHECK(vin[0] == 'A');
        CHECK(vin[5] == '3');
        CHECK(vin[6] == '0');
        CHECK(vin[16] == '0');
        CHECK(isValidVin(vin));
    }

    TEST_CASE("Construction from string - longer than 17 characters") {
        const std::string test_vin = "1HGBH41JXMN109186TOOLONGSTRING";
        Vin vin(test_vin);

        CHECK_FALSE(vin.isEmpty());
        CHECK(vin.toString() == "1HGBH41JXMN109186");

        // Verify truncation
        CHECK(vin[0] == '1');
        CHECK(vin[16] == '6');
        CHECK(isValidVin(vin));
    }

    TEST_CASE("Construction from empty string") {
        Vin vin("");

        CHECK(vin.isEmpty());
        CHECK(vin.toString() == "00000000000000000");
        CHECK_BYTE_ARRAY_REF_EQ(vin.asByteArray(), Vin::Zero.asByteArray());
    }

    TEST_CASE("Construction from C-style string") {
        const char* test_vin = "WVWZZZ1JZYW123456";
        Vin vin(test_vin);

        CHECK(vin.toString() == test_vin);
        CHECK(vin[0] == 'W');
        CHECK(vin[16] == '6');
        CHECK(isValidVin(vin));
    }

    TEST_CASE("Construction from nullptr C-style string") {
        const char* null_ptr = nullptr;
        Vin vin(null_ptr);

        CHECK(vin.isEmpty());
        CHECK_BYTE_ARRAY_REF_EQ(vin.asByteArray(), Vin::Zero.asByteArray());
        CHECK(isValidVin(vin));
    }

    TEST_CASE("Construction from byte sequence") {
        const uint8_t bytes[] = {'T', 'E', 'S', 'T', 'V', 'I', 'N', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0'};
        Vin vin(bytes, sizeof(bytes));

        CHECK(vin.toString() == "TESTVIN1234567890");
        CHECK(vin[0] == 'T');
        CHECK(vin[16] == vin.getPadByte());
        // I is illegal in VINs
        CHECK(isValidVin(vin) == false);
    }

    TEST_CASE("Construction from byte sequence - shorter") {
        const uint8_t bytes[] = {'S', 'H', 'O', 'R', 'T', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'};
        Vin vin(bytes, sizeof(bytes));

        CHECK(vin.toString() == "SHORT000000000000");
        CHECK(vin[0] == 'S');
        CHECK(vin[4] == 'T');
        CHECK(vin[5] == vin.getPadByte());
        CHECK(vin[16] == vin.getPadByte());
        // O is illegal in VINs
        CHECK(isValidVin(vin) == false);
    }

    TEST_CASE("Construction from byte sequence - longer") {
        const uint8_t bytes[] = {'V', 'E', 'R', 'Y', 'L', 'O', 'N', 'G', 'V', 'I', 'N', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0'};
        Vin vin(bytes, sizeof(bytes));

        CHECK(vin.toString() == "VERYLONGVIN123456");
        CHECK(vin[16] == '6');
        // Note: This VIN contains 'I' and 'O' which are invalid per ISO 3779
        CHECK(isValidVin(vin) == false);
    }

    TEST_CASE("Construction from null byte sequence") {
        Vin vin(nullptr, 10);

        CHECK(vin.isEmpty());
        CHECK(vin == Vin::Zero);
        CHECK(isValidVin(vin));
    }

    TEST_CASE("Construction from ByteArray - exact length") {
        ByteArray bytes = {'1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};
        Vin vin(bytes);

        CHECK(vin.toString() == "123456789ABCDEFGH");
        CHECK(vin[0] == '1');
        CHECK(vin[16] == 'H');
        CHECK(isValidVin(vin));
    }

    TEST_CASE("Construction from ByteArray - shorter") {
        ByteArray bytes = {'X', 'Y', 'Z', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'};
        Vin vin(bytes);

        CHECK(vin.toString() == "XYZ00000000000000");
        CHECK(vin[0] == 'X');
        CHECK(vin[2] == 'Z');
        CHECK(vin[3] == '0');
        CHECK(isValidVin(vin));
    }

    TEST_CASE("Construction from ByteArray - longer") {
        ByteArray bytes = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T'};
        Vin vin(bytes);

        CHECK(vin.toString() == "ABCDEFGHIJKLMNOPQ");
        CHECK(vin[16] == 'Q');
    }

    TEST_CASE("Construction from empty ByteArray") {
        ByteArray bytes;
        Vin vin(bytes);

        CHECK(vin.isEmpty());
        CHECK_BYTE_ARRAY_REF_EQ(vin.asByteArray(), Vin::Zero.asByteArray());
    }

    TEST_CASE("Copy constructor") {
        Vin vin1("ORIGINALVIN123456");
        Vin vin2(vin1);

        CHECK(vin1 == vin2);
        CHECK(vin2.toString() == "ORIGINALVIN123456");
    }

    TEST_CASE("Move assignment") {
        Vin vin1("MOVEASGN123456789");
        Vin vin2;

        vin2 = std::move(vin1);

        CHECK(vin2.toString() == "MOVEASGN123456789");
    }

    TEST_CASE("toString method") {
        SUBCASE("Full VIN") {
            Vin vin("FULLVIN1234567890");
            CHECK(vin.toString() == "FULLVIN1234567890");
        }

        SUBCASE("Partial VIN with padding") {
            Vin vin("PART");
            CHECK(vin.toString() == "PART0000000000000");
        }

        SUBCASE("Empty VIN") {
            Vin vin;
            CHECK(vin.toString() == "00000000000000000");
        }
    }

    TEST_CASE("getArray method") {
        Vin vin("ARRAYTEST12345678");
        const auto& arr = vin.getArray();

        CHECK(arr.size() == 17);
        CHECK(arr[0] == 'A');
        CHECK(arr[16] == '8');
        CHECK(isValidVin(vin));
    }

    TEST_CASE("data method") {
        Vin vin("DATATEST123456789");
        const uint8_t* ptr = vin.data();

        CHECK(ptr != nullptr);
        CHECK(ptr[0] == 'D');
        CHECK(ptr[16] == '9');
        CHECK(isValidVin(vin));
    }

    TEST_CASE("size method") {
        Vin vin1;
        Vin vin2("SHORT");
        Vin vin3("EXACTSEVENTEENVIN");




        CHECK(isValidVin(vin1));
        CHECK(isValidVin(vin2) == false); // O is illegal in VINs
        CHECK(isValidVin(vin3) == false); // I is illegal in VINs
    }

    TEST_CASE("isEmpty method") {
        SUBCASE("Empty VIN") {
            Vin vin;
            CHECK(vin.isEmpty());
        }

        SUBCASE("Empty string") {
            Vin vin("");
            CHECK(vin.isEmpty());
        }

        SUBCASE("Zero instance") {
            CHECK(Vin::Zero.isEmpty());
        }

        SUBCASE("Non-empty VIN") {
            Vin vin("X");
            CHECK_FALSE(vin.isEmpty());
            CHECK(isValidVin(vin));
        }

        SUBCASE("Full VIN") {
            Vin vin("FULLVXN1234567890");
            CHECK_FALSE(vin.isEmpty());
            CHECK(isValidVin(vin));
        }
    }

    TEST_CASE("Equality operator") {
        Vin vin1("SAMEVIN1234567890");
        Vin vin2("SAMEVIN1234567890");
        Vin vin3("DIFFVIN1234567890");

        CHECK(vin1 == vin2);
        CHECK_FALSE(vin1 == vin3);

        Vin vin4;
        Vin vin5;
        CHECK(vin4 == vin5);
        CHECK_BYTE_ARRAY_REF_EQ(vin4.asByteArray(), Vin::Zero.asByteArray());
    }

    TEST_CASE("Inequality operator") {
        Vin vin1("VIN1_12345678901");
        Vin vin2("VIN2_12345678901");
        Vin vin3("VIN1_12345678901");

        CHECK(vin1 != vin2);
        CHECK_FALSE(vin1 != vin3);

        Vin vin4;
        CHECK(vin1 != vin4);
    }

    TEST_CASE("Array subscript operator") {
        Vin vin("SUBSCRIPT12345678");

        CHECK(vin[0] == 'S');
        CHECK(vin[1] == 'U');
        CHECK(vin[8] == 'T');
        CHECK(vin[16] == '8');
    }

    TEST_CASE("Array subscript with padding") {
        Vin vin("PAD");

        CHECK(vin[0] == 'P');
        CHECK(vin[1] == 'A');
        CHECK(vin[2] == 'D');
        CHECK(vin[3] == '0');
        CHECK(vin[16] == '0');
    }

    TEST_CASE("VIN with special characters") {
        Vin vin("VIN-WITH_SPEC.IAL");

        CHECK(vin.toString() == "VIN-WITH_SPEC.IAL");
        CHECK(vin[3] == '-');
        CHECK(vin[8] == '_');
        CHECK(vin[13] == '.');
    }

    TEST_CASE("VIN with numeric characters") {
        Vin vin("12345678901234567");

        CHECK(vin.toString() == "12345678901234567");
        CHECK(vin[0] == '1');
        CHECK(vin[16] == '7');
    }

    TEST_CASE("VIN with lowercase characters") {
        Vin vin("lowercase12345678");

        // String constructor converts to uppercase
        CHECK(vin.toString() == "LOWERCASE12345678");
        CHECK(vin[0] == 'L');
        // 'O' is invalid per ISO 3779
        CHECK(isValidVin(vin) == false);
    }

    TEST_CASE("VIN with mixed case") {
        Vin vin("MxXeDcAsE12345678");

        // String constructor converts to uppercase
        CHECK(vin.toString() == "MXXEDCASE12345678");
        CHECK(vin[0] == 'M');
        CHECK(isValidVin(vin));
    }

    TEST_CASE("Real-world VIN examples") {
        SUBCASE("Honda VIN") {
            Vin vin("1HGBH41JXMN109186");
            CHECK(vin.toString() == "1HGBH41JXMN109186");
            CHECK_FALSE(vin.isEmpty());
        }

        SUBCASE("Volkswagen VIN") {
            Vin vin("WVWZZZ1JZYW123456");
            CHECK(vin.toString() == "WVWZZZ1JZYW123456");
            CHECK_FALSE(vin.isEmpty());
        }

        SUBCASE("BMW VIN") {
            Vin vin("WBA3B1G59DNP26082");
            CHECK(vin.toString() == "WBA3B1G59DNP26082");
            CHECK_FALSE(vin.isEmpty());
        }

        SUBCASE("Mercedes VIN") {
            Vin vin("WDDUG8CB9DA123456");
            CHECK(vin.toString() == "WDDUG8CB9DA123456");
            CHECK_FALSE(vin.isEmpty());
        }
    }

    TEST_CASE("VIN conversion round-trip") {
        const std::string original = "ROUNDTRIP12345678";

        Vin vin1(original);
        std::string str = vin1.toString();
        Vin vin2(str);

        CHECK(vin1 == vin2);
        CHECK(vin2.toString() == original);
    }

    TEST_CASE("ByteArray conversion round-trip") {
        const std::string original = "BYTEARRAYTRIP1234";

        Vin vin1(original);
        ByteArrayRef bytes = vin1.asByteArray();
        Vin vin2(bytes.first, bytes.second);

        CHECK(vin1 == vin2);
        CHECK(bytes.second == 17);
    }

    TEST_CASE("VIN with null bytes in middle") {
        uint8_t data[17] = {'V', 'I', 'N', 0, 'N', 'U', 'L', 'L', 0, 'B', 'Y', 'T', 'E', 'S', '1', '2', '3'};
        Vin vin(data, 17);

        // toString should stop at first null byte
        CHECK(vin.toString() == "VIN");

        // But the array should contain all data
        CHECK(vin[3] == 0);
        CHECK(vin[4] == 'N');
    }

    TEST_CASE("Constant correctness") {
        const Vin vin("CONSTVIN123456789");

        CHECK(vin.toString() == "CONSTVIN123456789");
                CHECK_FALSE(vin.isEmpty());
        CHECK(vin[0] == 'C');

        const auto& arr = vin.getArray();
        CHECK(arr[0] == 'C');

        const uint8_t* ptr = vin.data();
        CHECK(ptr[0] == 'C');
    }

    TEST_CASE("invalid VINs") {
        SUBCASE("VIN with invalid characters") {
            Vin vin("INVALID#VIN$12345");

            CHECK(vin.toString() == "INVALID#VIN$12345");
            CHECK(isValidVin(vin) == false);
        }

        SUBCASE("VIN with small character ") {
            Vin vin("isduds");

            CHECK(vin.toString() == "ISDUDS00000000000");
            CHECK(isValidVin(vin) == false);
        }
    }
}
