# cmd/iems/CMakeLists.txt

set(PROD_NAME iems)

add_library(iems_app_lib STATIC
    iems_node.c
    iems_driver_adapter.c
    iems_microgrid_coordinator.c
    iems_pcs_controller.c
    iems_bms_cluster_engine.c
    iems_multi_pcs_dispatcher.c
    iems_fire_safety_interlock.c
    iems_commissioning_tool.c
    iems_task_loader.c
    mqtt_producer.c
)

target_include_directories(iems_app_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/modules/edge_bootstrap/include
    ${CMAKE_SOURCE_DIR}/modules/edge_io/include
    ${CMAKE_SOURCE_DIR}/modules/edge_stream/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
    ${CMAKE_SOURCE_DIR}/modules/edge_protocol/include
    ${CMAKE_SOURCE_DIR}/modules/edge_security/include
    ${CMAKE_SOURCE_DIR}/modules/edge_infra/include
    ${CMAKE_SOURCE_DIR}/modules/edge_algo/include
)

target_link_libraries(iems_app_lib PUBLIC
    edge_bootstrap
    edge_io
    edge_core
    edge_protocol
    edge_stream
    edge_infra
    edge_algo
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
    edge_io 
    -Wl,--whole-archive
    edge_stream 
    edge_algo
    -Wl,--no-whole-archive
    edge_security
    edge_core
    edge_protocol
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
    edge_mcp
    edge_infra
    edge_io
    edge_stream
    edge_algo
    edge_security
    edge_core
    edge_protocol
    SQLite::SQLite3
    PahoMQTT::PahoMQTT
    -Wl,--end-group
)
install(TARGETS iems-cli DESTINATION bin)

if(ENABLE_WEB_BUILD)
    add_web_product(iems "${CMAKE_SOURCE_DIR}/web/products/iems")
    add_dependencies(iems web_ui_iems)
endif()

# 部署仿真与测试清单 (World & Simulation Manifest)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/products/iems/generated")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/omnis_manifest.json" 
     DESTINATION "${CMAKE_BINARY_DIR}/products/iems/generated")


