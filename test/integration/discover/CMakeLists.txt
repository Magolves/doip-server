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
    COMMAND bash -c "$<TARGET_FILE:Discover_Server> --daemon && sleep 1.0"
)
set_tests_properties(Integration_StartDiscoverServer PROPERTIES
    FIXTURES_SETUP demo_server
    TIMEOUT 20
    LABELS integration
    RUN_SERIAL TRUE
)

# Wait until both UDP 13400 and TCP 13400 are bound before running client tests
# This prevents race conditions where tests start before the server is ready
# Simplified script that works reliably in CI environments
add_test(NAME Integration_WaitForServerReady
    COMMAND bash -c "
echo 'Waiting for server to bind ports...'
sleep 3

# First check if server process exists
if ! pgrep -f Discover_Server >/dev/null 2>&1; then
  echo 'ERROR: Discover_Server process not running!' >&2
  exit 1
fi

for i in {1..150}; do
  if command -v ss >/dev/null 2>&1; then
    # Linux: use ss
    udp_check=`ss -ulpn 2>/dev/null | grep -c ':13400'`
    tcp_check=`ss -tlpn 2>/dev/null | grep -c ':13400'`
  else
    # macOS/BSD: use netstat
    udp_check=`netstat -an 2>/dev/null | grep -c 'udp.*\\.13400'`
    tcp_check=`netstat -an 2>/dev/null | grep -c '\\.13400.*LISTEN'`
  fi

  if [ \$udp_check -gt 0 ] && [ \$tcp_check -gt 0 ]; then
    echo 'Server ready: both UDP and TCP port 13400 are bound'
    # Give server additional time to start sending announcements (important for sanitizers)
    sleep 2
    exit 0
  fi

  if [ \$i -eq 50 ]; then
    echo \"Still waiting... (UDP check: \$udp_check, TCP check: \$tcp_check)\"
    pgrep -f Discover_Server >/dev/null 2>&1 || echo 'WARNING: Server process died!'
  fi

  sleep 0.1
done

echo 'ERROR: Server not ready after 15 seconds (UDP: '\$udp_check', TCP: '\$tcp_check')' >&2
pgrep -f Discover_Server >/dev/null 2>&1 || echo 'Server process is NOT running' >&2
exit 1
"
)
set_tests_properties(Integration_WaitForServerReady PROPERTIES
    FIXTURES_REQUIRED demo_server
    FIXTURES_SETUP demo_ready
    TIMEOUT 20
    LABELS integration
    RUN_SERIAL TRUE
)

# Integration test: run discover (udp client) against the running server
# CRITICAL: Must run serially and immediately after server is ready to catch announcements
add_test(NAME Integration_DiscoverClient_Runs
    COMMAND $<TARGET_FILE:Discover_Client>
)
set_tests_properties(Integration_DiscoverClient_Runs PROPERTIES
    FIXTURES_REQUIRED demo_ready
    TIMEOUT 20
    LABELS integration
    RUN_SERIAL TRUE
    DEPENDS Integration_WaitForServerReady
)

# Integration test: run tcp client against the running server
add_test(NAME Integration_DiagSessionRuns
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/test-local-client.py
)
set_tests_properties(Integration_DiagSessionRuns PROPERTIES
    FIXTURES_REQUIRED demo_ready
    TIMEOUT 20
    LABELS integration
    RUN_SERIAL TRUE
)

# Teardown fixture: stop server and wait for ports to be released
add_test(NAME Integration_StopDiscoverServer
    COMMAND bash -c "if [ -f /tmp/doip-discover.pid ]; then kill $(cat /tmp/doip-discover.pid) 2>/dev/null; rm -f /tmp/doip-discover.pid; else pkill -f Discover_Server 2>/dev/null || true; fi; for i in $(seq 1 50); do if command -v ss >/dev/null 2>&1; then ss -tlpn 2>/dev/null | grep -q ':13400' || exit 0; else netstat -an 2>/dev/null | grep -q '\\.13400.*LISTEN' || exit 0; fi; sleep 0.1; done; echo 'Warning: Port 13400 still in use' >&2; exit 0"
)
set_tests_properties(Integration_StopDiscoverServer PROPERTIES
    FIXTURES_CLEANUP demo_server
    TIMEOUT 10
    LABELS integration
    RUN_SERIAL TRUE
)
