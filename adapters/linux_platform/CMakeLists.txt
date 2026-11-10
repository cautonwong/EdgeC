add_library(edge_linux_platform STATIC
    src/io_executor_linux.c
    src/edge_tsn_engine.c
    src/edge_bsp_hal.c
)

target_include_directories(edge_linux_platform PUBLIC
    ${CMAKE_SOURCE_DIR}/modules/edge_io/include
    ${CMAKE_SOURCE_DIR}/modules/edge_io/src
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
    ${CMAKE_SOURCE_DIR}/foundation/include
)

target_link_libraries(edge_linux_platform PUBLIC
    edge_core
    edge_log
    edge_netbuf
)
target_link_libraries(edge_linux_platform PRIVATE libuv::uv_a CURL::libcurl PahoMQTT::PahoMQTT)
