# ==============================================================================
# EdgeOS Infrastructure & Driver Subsystem (edge_infra)
# ==============================================================================

add_library(edge_infra STATIC
    storage/sqlite_service.c
    storage/sqlite_storage_driver.c
    bus_dds/datahub_dds.c
    bus_mqtt/datahub_mqtt.c
)

target_include_directories(edge_infra PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)

target_link_libraries(edge_infra PUBLIC edge_common_interface cyclonedds)
target_link_libraries(edge_infra PRIVATE libuv::uv_a SQLite::SQLite3 zlog::zlog PahoMQTT::PahoMQTT)
