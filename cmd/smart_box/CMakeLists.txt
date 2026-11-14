set(PROD_NAME smart_box)

add_library(smartbox_app_lib STATIC
    smartbox_node.c
    smartbox_driver_adapter.c
    smartbox_five_safety_interlock.c
    smartbox_thermal_aging.c
    smartbox_north_adapter.c
    smartbox_hw_io_adapter.c
    smartbox_peripheral_adapter.c
    smartbox_ble_adapter.c
    smartbox_uds_ipc.c
)
target_include_directories(smartbox_app_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/modules/edge_bootstrap/include
    ${CMAKE_SOURCE_DIR}/modules/edge_io/include
    ${CMAKE_SOURCE_DIR}/modules/edge_north/include
    ${CMAKE_SOURCE_DIR}/adapters/mqtt_transport/include
    ${CMAKE_SOURCE_DIR}/modules/edge_stream/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
    ${CMAKE_SOURCE_DIR}/modules/edge_infra/include
)
target_link_libraries(smartbox_app_lib PUBLIC
    edge_bootstrap
    edge_io
    edge_io_drivers
    edge_north
    mqtt_transport
    edge_core
    edge_codec
    edge_stream
    edge_linux_platform
    edge_infra
    edge_ai
    edge_battery
    edge_health
    edge_power
    edge_runtime
)

add_executable(smart_box main.c)
set_target_properties(smart_box PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/products/smart_box/bin"
)
target_link_libraries(smart_box PRIVATE 
    smartbox_app_lib
    -Wl,--start-group
    edge_bootstrap
    edge_infra
    edge_io
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
    -Wl,--end-group
    crypto
    ssl
)

if(ENABLE_WEB_BUILD)
    add_web_product(smart_box "${CMAKE_SOURCE_DIR}/web/products/smart_box")
    add_dependencies(smart_box web_ui_smart_box)
endif()
