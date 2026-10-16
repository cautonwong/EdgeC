set(PROD_NAME smart_box)
edge_add_contract(
    contract_${PROD_NAME}
    "${CMAKE_SOURCE_DIR}/contracts/products/smart_box.yaml"
    "${CMAKE_CURRENT_SOURCE_DIR}/manifest.yaml"
)
add_executable(smart_box main.c)
set_target_properties(smart_box PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/products/smart_box/bin"
)
target_link_libraries(smart_box PRIVATE 
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
    contract_smart_box
    crypto
    ssl
)

if(ENABLE_WEB_BUILD)
    add_web_product(smart_box "${CMAKE_SOURCE_DIR}/web/products/smart_box")
    add_dependencies(smart_box web_ui_smart_box)
endif()
