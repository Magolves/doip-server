# Examples CMakeLists.txt

set (DEMO_SOURCES
    SimpleServer.cpp
    Discover.cpp
)

foreach(demo_source ${DEMO_SOURCES})
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

# TODO: Move to test CMakeLists.txt
if (WITH_UNIT_TEST)
    message(STATUS "Adding integration tests for examples")

    # Integration fixture: start DoIPServer as a daemon
    add_test(NAME Integration_StartSimpleServer
        COMMAND $<TARGET_FILE:SimpleServer> --daemonize --loopback
    )
    set_tests_properties(Integration_StartSimpleServer PROPERTIES
        FIXTURES_SETUP demo_server
        TIMEOUT 20
        LABELS integration
    )

    # Integration test: run discover (udp client) against the running server
    add_test(NAME Integration_DiscoverRuns
       COMMAND $<TARGET_FILE:Discover> --loopback
    )
    set_tests_properties(Integration_DiscoverRuns PROPERTIES
        FIXTURES_REQUIRED demo_server
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

    # Teardown fixture: stop DoIPServer
    add_test(NAME Integration_StopSimpleServer
       COMMAND pkill -f SimpleServer
    )
    set_tests_properties(Integration_StopSimpleServer PROPERTIES
        FIXTURES_CLEANUP demo_server
        TIMEOUT 10
        LABELS integration
    )
endif()
