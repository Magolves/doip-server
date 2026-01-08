
add_executable(${DOIP_SERVER_LIB_NAME}_tests
    DoIPDefaultConnection_Test.cpp
    DoIPMessage_Test.cpp
    DoIPServer_Test.cpp
    Identifiers_Test.cpp
    Main_Test.cpp
    Misc_Test.cpp
    Stream_Test.cpp
    ThreadSafeQueue_Test.cpp
    TimerManager_Test.cpp
    VehicleIdentification_Test.cpp
    cli/ServerConfigCLI_Test.cpp
    tp/MockTransport_Test.cpp
    tp/MockServerTransport_Test.cpp
    uds/UdsMockProvider_Test.cpp
)

target_link_libraries(${DOIP_SERVER_LIB_NAME}_tests
    PRIVATE
    doctest::doctest
    doctest::doctest-aux
    ${DOIP_SERVER_LIB_NAME}
)

# Configure relaxed clang-tidy settings for tests (disable magic-numbers and nodiscard warnings)
configure_clang_tidy_for_tests(${DOIP_SERVER_LIB_NAME}_tests)

# Use automatic test discovery instead of manual add_test
doctest_discover_tests(${DOIP_SERVER_LIB_NAME}_tests
    PROPERTIES
        LABELS "${DOIP_SERVER_LIB_NAME}"
        TIMEOUT 30
)
