set(UDS_LIB_NAME "doip-uds")
set(UDS_SOURCES
    src/UdsMock.cpp
    src/UdsDiagnosticTroubleCode.cpp
    src/services/UdsSecurityAccess.cpp
    src/services/UdsReadDTCInformation.cpp
)

# Configure configuration header
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/inc/uds/gen/UdsConfig.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/inc/uds/gen/UdsConfig.h
    @ONLY
)


# Create the library
add_library(${UDS_LIB_NAME}
    ${UDS_SOURCES}
)

add_library(doip::uds ALIAS ${UDS_LIB_NAME})

# Set target properties
set_target_properties(${UDS_LIB_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Include directories
target_include_directories(${UDS_LIB_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/lib${UDS_LIB_NAME}>
)

# Link dependencies
target_link_libraries(${UDS_LIB_NAME}
    PUBLIC
        doip::util
)

# Disable switch-default warning for this target since spdlog headers trigger it
target_compile_options(${UDS_LIB_NAME} PRIVATE -Wno-switch-default)
if(WITH_TEST_COV)
    # Instrument library for coverage
    message(STATUS "Instrumenting ${UDS_LIB_NAME} for test coverage")
    target_compile_options(${UDS_LIB_NAME} PRIVATE --coverage)
    # Use PUBLIC link options so coverage libraries propagate to consumers
    target_link_libraries(${UDS_LIB_NAME} PUBLIC gcov)
endif()

if (WITH_UNIT_TEST)
    include(${CMAKE_SOURCE_DIR}/cmake/doctest.cmake)
    add_subdirectory(test)
endif()

# Install libraries
install(TARGETS ${UDS_LIB_NAME}
    EXPORT lib${UDS_LIB_NAME}Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Create and install package config files
install(EXPORT lib${UDS_LIB_NAME}Targets
    FILE lib${UDS_LIB_NAME}Targets.cmake
    NAMESPACE lib${UDS_LIB_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lib${UDS_LIB_NAME}
)
# Install public headers
install(DIRECTORY inc/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/lib${UDS_LIB_NAME}
)