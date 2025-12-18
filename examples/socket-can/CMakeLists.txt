# Examples CMakeLists.txt

set (DEMO_SOURCES
    DoIPCanIsoTpServer.cpp
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
    )

    # Disable switch-default warning for examples using spdlog
    target_compile_options(${demo_name} PRIVATE -Wno-switch-default)

    # Install examples (optional)
    install(TARGETS ${demo_name}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/examples
    )

endforeach()
