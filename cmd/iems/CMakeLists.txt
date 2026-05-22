# cmd/iems/CMakeLists.txt

set(PROD_NAME iems)

# 签名: edge_add_contract(TARGET_NAME PRODUCT_YAML LOCAL_YAML) (以接地气的中文添加注释)
edge_add_contract(
    contract_${PROD_NAME}
    "${CMAKE_SOURCE_DIR}/contracts/products/iems_v5.yaml"
    "${CMAKE_CURRENT_SOURCE_DIR}/manifest.yaml"
)

add_executable(iems main.c)
set_target_properties(iems PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/products/iems/bin"
)
target_link_libraries(iems PRIVATE 
    -Wl,--start-group
    edge_io 
    -Wl,--whole-archive
    edge_stream 
    edge_algo
    -Wl,--no-whole-archive
    edge_security
    edge_core
    edge_protocol
    -Wl,--end-group
    contract_iems
)
install(TARGETS iems DESTINATION bin)

