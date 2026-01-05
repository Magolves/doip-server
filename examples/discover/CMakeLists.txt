add_executable(DoIPUdpServer DoIPUdpServer.cpp)
target_link_libraries(DoIPUdpServer
    PRIVATE
    ${DOIP_SERVER_LIB_NAME}
)

configure_file(discover-client.py ${CMAKE_CURRENT_BINARY_DIR}/discover-client.py COPYONLY)
configure_file(discover-client.py ${CMAKE_BINARY_DIR}/discover-client.py COPYONLY)