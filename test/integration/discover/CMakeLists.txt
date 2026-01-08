# Examples CMakeLists.txt

set (DISCOVER_SOURCES
    DiscoverServer.cpp
    DiscoverClient.cpp
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
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
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

if (WITH_INTEGRATION_TEST)
    message(STATUS "Building Discover C++ Client integration test in ${CMAKE_CURRENT_BINARY_DIR}")
    configure_file(run-test.sh ${CMAKE_CURRENT_BINARY_DIR}/run-test.sh COPYONLY)

    add_custom_target(
        copy_test_script_discover ALL
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/run-test.sh
    )

    add_test(
        NAME Integration_UdpDiscoverCpp
        COMMAND bash ${CMAKE_CURRENT_BINARY_DIR}/run-test.sh
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
    set_tests_properties(Integration_UdpDiscoverCpp PROPERTIES
        TIMEOUT 20
        DEPENDS copy_test_script_discover_cs
        LABELS "integration"
    )
endif()