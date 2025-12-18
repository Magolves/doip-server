# Demos CMakeLists.txt
add_subdirectory(discover)

if (LINUX)
    add_subdirectory(socket-can)
endif()

