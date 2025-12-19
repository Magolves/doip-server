# Add subdirectories first to create test targets
add_subdirectory(unit)
add_subdirectory(integration)

# ------------------------------------------------------------
# Optional: Test coverage task with HTML report (requires gcovr)
# ------------------------------------------------------------
find_program(GCOVR_PATH NAMES gcovr)
set(GCOVR_FOUND OFF)
if(GCOVR_PATH)
    set(GCOVR_FOUND ON)
    message(STATUS "gcovr found: ${GCOVR_PATH}")

    if(WITH_TEST_COV)
        message(STATUS "WITH_TEST_COV enabled")

        # Instrument tests and library for coverage
        target_compile_options(${DOIP_SERVER_LIB_NAME}_tests PRIVATE --coverage)
        target_link_options(${DOIP_SERVER_LIB_NAME}_tests PRIVATE --coverage)

        # Create custom target to run tests and generate HTML coverage report
        add_custom_target(test_cov
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/test/reports
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -C $<CONFIG>
            COMMAND ${GCOVR_PATH} --root ${CMAKE_SOURCE_DIR} --filter ${CMAKE_SOURCE_DIR}/src --filter ${CMAKE_SOURCE_DIR}/inc --exclude ${CMAKE_BINARY_DIR} --exclude ${CMAKE_SOURCE_DIR}/test --html --html-details --output ${CMAKE_SOURCE_DIR}/test/reports/coverage.html
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Run tests with coverage and generate HTML report: test/reports/coverage.html"
        )
    else()
        message(STATUS "WITH_TEST_COV disabled")
    endif()
else()
    if(WITH_TEST_COV)
        message(WARNING "WITH_TEST_COV enabled but gcovr not found! Test coverage target will not be created.")
    else()
        message(STATUS "WITH_TEST_COV disabled")
    endif()
endif()
