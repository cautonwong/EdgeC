# cmd/nexus_lab/CMakeLists.txt

add_executable(nexus_lab main.c)
target_link_libraries(nexus_lab PRIVATE 
    -Wl,--start-group
    edge_ai
    edge_battery
    edge_health
    edge_power
    edge_runtime
    edge_io 
    edge_stream 
    edge_infra
    -Wl,--end-group
)
install(TARGETS nexus_lab DESTINATION bin)
