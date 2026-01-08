# Demos CMakeLists.txt
if (LINUX)
    add_subdirectory(socket-can)
endif()

add_subdirectory(minimal)
add_subdirectory(discover)

