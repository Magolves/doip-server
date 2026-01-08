# Examples CMakeLists.txt

set (DISCOVER_SOURCES
    UdsServer.cpp
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

endforeach()


# Define the source and destination paths
set(SOURCE_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/test-uds-download.py)
set(DESTINATION_SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/test-uds-download.py)
configure_file(${SOURCE_SCRIPT} ${DESTINATION_SCRIPT} COPYONLY)
configure_file(run-test.sh ${CMAKE_CURRENT_BINARY_DIR}/run-test.sh COPYONLY)

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
add_custom_target(copy_uds_download_script ALL DEPENDS ${DESTINATION_SCRIPT})

message(STATUS "Adding integration tests for examples")

if (WITH_INTEGRATION_TEST)
    add_test(
        NAME uds-download-test
        COMMAND bash ${CMAKE_CURRENT_BINARY_DIR}/run-test.sh
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
    set_tests_properties(uds-download-test PROPERTIES
        DEPENDS "setup_venv;copy_uds_download_script"
    )
endif()