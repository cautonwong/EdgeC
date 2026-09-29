# cmd/nexus_lab/CMakeLists.txt

add_executable(nexus_lab main.c)
target_link_libraries(nexus_lab PRIVATE 
    -Wl,--start-group
    edge_algo
    edge_io 
    edge_stream 
    edge_security
    -Wl,--end-group
)
install(TARGETS nexus_lab DESTINATION bin)
