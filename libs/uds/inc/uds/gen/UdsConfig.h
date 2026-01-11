#pragma once

#include <cstdint>
#include <unistd.h>

namespace doip::uds {

constexpr const char* UDS_VERSION = "1.0.0";
constexpr const char* UDS_VERSION_MAJOR = "1";
constexpr const char* UDS_VERSION_MINOR = "0";
constexpr const char* UDS_VERSION_PATCH = "0";

constexpr const char* UDS_BUILD_DATE = "";
constexpr const char* UDS_BUILD_TYPE = "Debug";

/**
 * @brief Constants for UDS Security Access seed/key algorithm.
 */
#ifndef UDS_CONFIG_SEED_CONSTANT_1
static constexpr uint32_t UDS_CONFIG_SEED_CONSTANT_1 = 0xA5A5A5A5;
#endif

#ifndef UDS_CONFIG_SEED_OFFSET_1
static constexpr uint32_t UDS_CONFIG_SEED_OFFSET_1 = 0x12345678;
#endif

#ifndef UDS_CONFIG_SEED_MASK_2
static constexpr uint32_t UDS_CONFIG_SEED_MASK_2 = 0x5A5A5A5A;
#endif

#ifndef UDS_CONFIG_SEED_MULTIPLIER
static constexpr uint32_t UDS_CONFIG_SEED_MULTIPLIER = 0x9D2C5680;
#endif

#ifndef UDS_CONFIG_SEED_OFFSET_2
static constexpr uint32_t UDS_CONFIG_SEED_OFFSET_2 = 0x87654321;
#endif

} // namespace doip::uds
