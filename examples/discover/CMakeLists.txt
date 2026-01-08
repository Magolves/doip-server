add_executable(DoIPUdpServer DoIPUdpServer.cpp)
target_link_libraries(DoIPUdpServer
    PRIVATE
    ${DOIP_SERVER_LIB_NAME}
)

set_target_properties(DoIPUdpServer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

configure_file(discover-client.py ${CMAKE_CURRENT_BINARY_DIR}/discover-client.py COPYONLY)
configure_file(discover-client.py ${CMAKE_BINARY_DIR}/discover-client.py COPYONLY)

set(SOURCE_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/discover-client.py)
set(DESTINATION_SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/discover-client.py)
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
add_custom_target(copy_discover_script ALL DEPENDS ${DESTINATION_SCRIPT})


if (WITH_INTEGRATION_TEST)
    add_test(
        NAME Integration_UdpDiscover
        COMMAND bash ${CMAKE_CURRENT_BINARY_DIR}/run-test.sh
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
    set_tests_properties(Integration_UdpDiscover PROPERTIES
        DEPENDS "setup_venv;copy_discover_script"
    )
endif()