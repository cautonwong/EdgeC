set(PROD_NAME smart_box)

add_library(smartbox_app_lib STATIC
    smartbox_node.c
    smartbox_driver_adapter.c
    smartbox_five_safety_interlock.c
    smartbox_thermal_aging.c
)
target_include_directories(smartbox_app_lib PUBLIC
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
target_link_libraries(smartbox_app_lib PUBLIC
    edge_bootstrap
    edge_io
    edge_core
    edge_protocol
    edge_stream
    edge_infra
    edge_algo
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
    -Wl,--whole-archive
    edge_stream
    edge_algo
    -Wl,--no-whole-archive
    edge_security
    edge_core
    edge_protocol
    -Wl,--end-group
    crypto
    ssl
)

if(ENABLE_WEB_BUILD)
    add_web_product(smart_box "${CMAKE_SOURCE_DIR}/web/products/smart_box")
    add_dependencies(smart_box web_ui_smart_box)
endif()
