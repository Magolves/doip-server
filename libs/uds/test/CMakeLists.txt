
add_executable(${UDS_LIB_NAME}-tests
    Main_Test.cpp
    UdsDiagnosticTroubleCode_Test.cpp
    Vin_Test.cpp
    UdsMock_Test.cpp
    UdsSecurityAccess_Test.cpp
)

target_link_libraries(${UDS_LIB_NAME}-tests
    PRIVATE
        doip::util
        doip::uds
        doctest::doctest
        doctest::doctest-aux
)

# Configure relaxed clang-tidy settings for tests (disable magic-numbers and nodiscard warnings)
configure_clang_tidy_for_tests(${UDS_LIB_NAME}-tests)

# Use automatic test discovery instead of manual add_test
doctest_discover_tests(${UDS_LIB_NAME}-tests
    PROPERTIES
        LABELS "${UDS_LIB_NAME}"
        TIMEOUT 30
)
