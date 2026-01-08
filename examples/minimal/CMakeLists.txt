# Examples CMakeLists.txt

set (DEMO_SOURCES
    MinimalDoIPServer.cpp
    MinimalDoIPClient.cpp
)

foreach(demo_source ${DEMO_SOURCES})
    get_filename_component(demo_name ${demo_source} NAME_WE)
    add_executable(${demo_name} ${demo_source})
    target_link_libraries(${demo_name}
        PRIVATE
        ${DOIP_SERVER_LIB_NAME}
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
endforeach()

if (WITH_INTEGRATION_TEST)
    configure_file(run-test.sh ${CMAKE_CURRENT_BINARY_DIR}/run-test.sh COPYONLY)

    add_custom_target(
        copy_script_minimal ALL
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/run-test.sh
    )

    add_test(
        NAME uds-minimal-test
        COMMAND bash ${CMAKE_CURRENT_BINARY_DIR}/run-test.sh
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
    set_tests_properties(uds-minimal-test PROPERTIES
        TIMEOUT 20
        DEPENDS copy_script_minimal
        LABELS "integration"
    )
endif()