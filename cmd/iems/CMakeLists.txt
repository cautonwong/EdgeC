# cmd/iems/CMakeLists.txt

set(PROD_NAME iems)

# 厂商语义 vtable 框架：只依赖消费者定义的 tag I/O 端口
add_library(iems_vendor STATIC
    vendor/iems_vendor_semantics.c
    vendor/iems_vendor_pcs_a.c
    vendor/iems_vendor_pcs_b.c
    vendor/iems_vendor_nonpcs.c
)
target_include_directories(iems_vendor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/vendor)

# P1-02 点位模型骨架：点键槽位校验与已知名表，静态、无动态分配
add_library(iems_points STATIC
    points/iems_point_model.c
)
target_include_directories(iems_points PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/points)

add_library(iems_app_lib STATIC
    iems_node.c
    iems_bess_aggregate.c
    iems_app.c
    iems_health_monitor.c
    iems_pcs_settings.c
    iems_wal_start.c
    iems_app_manager_adapter.c
    iems_app_runtime.c
    iems_app_host_ports.c
    iems_pressure_adapter.c
    iems_execution_adapter.c
    iems_north_adapter.c
    iems_driver_adapter.c
    iems_microgrid_coordinator.c
    iems_tou_demand.c
    iems_pcs_controller.c
    iems_bms_cluster_engine.c
    iems_multi_pcs_dispatcher.c
    iems_fire_safety_interlock.c
    iems_commissioning_tool.c
    iems_alarm_registry.c
    iems_alarm_state.c
    iems_config_loader.c
    iems_bmser_ipc.c
    iems_task_loader.c
    iems_poll_scheduler.c
    iems_southbound_adapter.c
    iems_tcp_link.c
    iems_internal_bus_adapter.c
    iems_ui_channel.c
    iems_north_listener.c
    iems_north_iec104.c
)






target_include_directories(iems_app_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/modules/edge_bootstrap/include
    ${CMAKE_SOURCE_DIR}/adapters/mcp_industry/include
    ${CMAKE_SOURCE_DIR}/modules/edge_io/include
    ${CMAKE_SOURCE_DIR}/adapters/rest_api/include
    ${CMAKE_SOURCE_DIR}/modules/edge_north/include
    ${CMAKE_SOURCE_DIR}/adapters/mqtt_transport/include
    ${CMAKE_SOURCE_DIR}/modules/edge_stream/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
    ${CMAKE_SOURCE_DIR}/modules/edge_infra/include
    ${CMAKE_SOURCE_DIR}/modules/edge_mgmt/include
)

target_link_libraries(iems_app_lib PUBLIC
    cjson
    edge_bootstrap
    edge_io
    edge_io_drivers
    edge_api
    edge_rest_api
    edge_north
    mqtt_transport
    edge_core
    edge_codec
    edge_stream
    edge_infra
    edge_infra_security
    edge_ai
    edge_battery
    edge_health
    edge_power
    edge_runtime
    edge_mgmt
    edge_app_host
    edge_linux_platform
    iems_points
    iems_vendor
)

add_executable(iems main.c)
set_target_properties(iems PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/products/iems/bin"
)
target_link_libraries(iems PRIVATE 
    -Wl,--start-group
    iems_app_lib
    edge_bootstrap
    edge_infra
    edge_infra_security
    edge_io
    edge_rest_api
    edge_north
    -Wl,--whole-archive
    edge_stream 
    edge_ai
    edge_battery
    edge_health
    edge_power
    edge_runtime
    -Wl,--no-whole-archive
    edge_core
    edge_codec
    SQLite::SQLite3
    PahoMQTT::PahoMQTT
    -Wl,--end-group
)
install(TARGETS iems DESTINATION bin)

add_executable(iems-cli iems_cli.c)
set_target_properties(iems-cli PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/products/iems/bin"
)
target_link_libraries(iems-cli PRIVATE
    -Wl,--start-group
    iems_app_lib
    edge_bootstrap
    edge_mcp
    edge_mcp_industry
    edge_infra
    edge_infra_security
    edge_io
    edge_rest_api
    edge_north
    edge_stream
    edge_ai
    edge_battery
    edge_health
    edge_power
    edge_runtime
    edge_core
    edge_codec
    SQLite::SQLite3
    PahoMQTT::PahoMQTT
    -Wl,--end-group
)
install(TARGETS iems-cli DESTINATION bin)

if(ENABLE_WEB_BUILD)
    add_web_product(iems "${CMAKE_SOURCE_DIR}/web/products/iems")
    add_dependencies(iems web_ui_iems)
endif()


