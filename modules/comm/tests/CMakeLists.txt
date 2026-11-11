add_executable(comm_protobuf_datagram_test datagram_test.cpp)
target_link_libraries(comm_protobuf_datagram_test PRIVATE comm_protobuf)
add_test(NAME comm_protobuf_datagram COMMAND comm_protobuf_datagram_test)

# Sends real datagrams over the loopback interface; needs no network setup.
add_executable(comm_udp_loopback_test udp_loopback_test.cpp)
target_link_libraries(comm_udp_loopback_test PRIVATE comm_protobuf comm_udp)
add_test(NAME comm_udp_loopback COMMAND comm_udp_loopback_test)
