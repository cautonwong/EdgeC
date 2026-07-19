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
    edge_stream 
    edge_core
    edge_security
    edge_protocol
    contract_smart_box
)
