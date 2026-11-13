add_executable(app-host
    main.c
    ${CMAKE_SOURCE_DIR}/sdk/c/edge_app_sdk.c
)

target_include_directories(app-host PRIVATE ${CMAKE_SOURCE_DIR}/sdk/c)
target_link_libraries(app-host PRIVATE iwasm::vmlib)

install(TARGETS app-host
        DESTINATION libexec/edgeos
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_EXECUTE)
