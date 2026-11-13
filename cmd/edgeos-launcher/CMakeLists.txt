add_executable(edgeos-launcher main.c)

target_link_libraries(edgeos-launcher PRIVATE edge_foundation edge_common_interface)

install(TARGETS edgeos-launcher
        DESTINATION libexec/edgeos
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_EXECUTE)
