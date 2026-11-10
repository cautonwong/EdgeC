add_library(edge_session STATIC
    src/session_manager.c
)

target_include_directories(edge_session PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/adapters/io_drivers/include
    ${CMAKE_SOURCE_DIR}/foundation/include
)

target_link_libraries(edge_session PUBLIC
    edge_io_drivers
    edge_netbuf
    edge_log
)
