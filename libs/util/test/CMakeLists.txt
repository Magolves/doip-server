
add_executable(${UTIL_LIB_NAME}-tests
    ByteArray_Test.cpp
    MacAddress_Test.cpp
    Main_Test.cpp
    TimerManager_Test.cpp
)

target_link_libraries(${UTIL_LIB_NAME}-tests
    PRIVATE
        doip::util
        doctest::doctest
)

# Configure relaxed clang-tidy settings for tests (disable magic-numbers and nodiscard warnings)
configure_clang_tidy_for_tests(${UTIL_LIB_NAME}-tests)

# Use automatic test discovery instead of manual add_test
doctest_discover_tests(${UTIL_LIB_NAME}-tests
    PROPERTIES
        LABELS "${UTIL_LIB_NAME}"
        TIMEOUT 30
)
