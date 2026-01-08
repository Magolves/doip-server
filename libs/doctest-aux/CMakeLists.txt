add_library(doctest-aux INTERFACE)
add_library(doctest::doctest-aux ALIAS doctest-aux)
target_include_directories(doctest-aux INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/inc)