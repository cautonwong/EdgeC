add_executable(edgeos-backup main.c)

find_package(OpenSSL REQUIRED)

target_link_libraries(edgeos-backup PRIVATE edge_sqlite_adapters OpenSSL::Crypto)

install(TARGETS edgeos-backup
        DESTINATION bin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ
                    WORLD_EXECUTE)
