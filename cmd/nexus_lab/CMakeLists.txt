# cmd/nexus_lab/CMakeLists.txt

add_executable(nexus_lab main.c)
target_link_libraries(nexus_lab PRIVATE 
    edge_io 
    edge_stream 
    edge_security
)
install(TARGETS nexus_lab DESTINATION bin)
