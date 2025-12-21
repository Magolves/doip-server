# Examples CMakeLists.txt

set (DISCOVER_SOURCES
    Discover_Server.cpp
    Discover_Client.cpp
)

foreach(demo_source ${DISCOVER_SOURCES})
    get_filename_component(demo_name ${demo_source} NAME_WE)
    add_executable(${demo_name} ${demo_source})
    target_link_libraries(${demo_name}
        PRIVATE
        ${DOIP_SERVER_LIB_NAME}
        CLI11::CLI11
        Threads::Threads
    )

    # Set properties for the example
    set_target_properties(${demo_name} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )

    # Disable switch-default warning for examples using spdlog
    target_compile_options(${demo_name} PRIVATE -Wno-switch-default)
    if(WITH_TEST_COV)
        # Instrument library for coverage
        message(STATUS "Instrumenting ${demo_name} for test coverage")
        target_compile_options(${demo_name} PRIVATE --coverage)
        target_link_options(${demo_name} PRIVATE --coverage)
    endif()

    # Install examples (optional)
    install(TARGETS ${demo_name}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/examples
    )

endforeach()


# Define the source and destination paths
set(SOURCE_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/test-local-client.py)
set(DESTINATION_SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/test-local-client.py)

# Add a custom command to copy the file only if it changes
add_custom_command(
    OUTPUT ${DESTINATION_SCRIPT}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SOURCE_SCRIPT}
            ${DESTINATION_SCRIPT}
    DEPENDS ${SOURCE_SCRIPT}
    COMMENT "Copying ${SOURCE_SCRIPT} to binary directory (if changed)"
)

# Optionally, add the output to a target to ensure it's built
add_custom_target(copy_script ALL DEPENDS ${DESTINATION_SCRIPT})

message(STATUS "Adding integration tests for examples")

# Integration fixture: start DoIPServer as a daemon
add_test(NAME Integration_StartDiscoverServer
    COMMAND $<TARGET_FILE:Discover_Server> --daemon
)
set_tests_properties(Integration_StartDiscoverServer PROPERTIES
    FIXTURES_SETUP demo_server
    TIMEOUT 20
    LABELS integration
)

# Wait until UDP 13401 is bound before running client
add_test(NAME Integration_WaitForServerReady
    COMMAND bash -c "for i in $(seq 1 50); do ss -ulpn | grep 13401 && exit 0; sleep 0.1; done; echo 'Server not ready on UDP 13401' >&2; exit 1"
)
set_tests_properties(Integration_WaitForServerReady PROPERTIES
    FIXTURES_REQUIRED demo_server
    FIXTURES_SETUP demo_ready
    TIMEOUT 10
    LABELS integration
)

# Integration test: run discover (udp client) against the running server
# FIXME: No route to host error occurs here -> no announcement received by client
add_test(NAME Integration_DiscoverClient_Runs
    COMMAND $<TARGET_FILE:Discover_Client>
)
set_tests_properties(Integration_DiscoverClient_Runs PROPERTIES
    FIXTURES_REQUIRED "demo_server;demo_ready"
    TIMEOUT 20
    LABELS integration
)

# Integration test: run tcp client against the running server
add_test(NAME Integration_DiagSessionRuns
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/test-local-client.py
)
set_tests_properties(Integration_DiagSessionRuns PROPERTIES
    FIXTURES_REQUIRED demo_server
    TIMEOUT 20
    LABELS integration
)

# Teardown fixture: stop server
add_test(NAME Integration_StopDiscoverServer
    COMMAND bash -c "if [ -f /tmp/doip-discover.pid ]; then kill \$(cat /tmp/doip-discover.pid); rm -f /tmp/doip-discover.pid; else pkill -f Discover_Server; fi"
)
set_tests_properties(Integration_StopDiscoverServer PROPERTIES
    FIXTURES_CLEANUP demo_server
    TIMEOUT 10
    LABELS integration
)
