cmake_minimum_required(VERSION 3.9)
project(FlowExpirationTimeouts)

set(CMAKE_CXX_STANDARD 11)
add_compile_options(-W -Wall -Wextra -pedantic -pedantic-errors -O3 -g0)

add_executable(pcap_parser pcap-parser.cpp)
target_link_libraries(pcap_parser tins pcap lzma)

add_executable(compute_flows compute-flows.cpp)

message("-- Target compiler flags are: ${MAIN_CFLAGS}")