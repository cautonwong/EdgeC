add_executable(edgeos-pack main.c)

target_link_libraries(edgeos-pack PRIVATE edge_mgmt OpenSSL::Crypto)

install(TARGETS edgeos-pack
        DESTINATION bin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ
                    WORLD_EXECUTE)
