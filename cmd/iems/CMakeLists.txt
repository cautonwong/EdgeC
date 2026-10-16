# cmd/iems/CMakeLists.txt

set(PROD_NAME iems)

# 签名: edge_add_contract(TARGET_NAME PRODUCT_YAML LOCAL_YAML) (以接地气的中文添加注释)
edge_add_contract(
    contract_${PROD_NAME}
    "${CMAKE_SOURCE_DIR}/contracts/products/iems_v5.yaml"
    "${CMAKE_CURRENT_SOURCE_DIR}/manifest.yaml"
)

add_library(iems_app_lib STATIC
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
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
    ${CMAKE_SOURCE_DIR}/modules/edge_algo/include
)

target_link_libraries(iems_app_lib PUBLIC
    edge_core
    edge_algo
    contract_${PROD_NAME}
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
    contract_iems
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
    contract_iems
)
install(TARGETS iems-cli DESTINATION bin)

if(ENABLE_WEB_BUILD)
    add_web_product(iems "${CMAKE_SOURCE_DIR}/web/products/iems")
    add_dependencies(iems web_ui_iems)
endif()

