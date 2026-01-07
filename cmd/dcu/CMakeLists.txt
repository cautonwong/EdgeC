# EdgeOS DCU 编译配置 (修正参数顺序版)

set(PROD_NAME dcu)

# 签名: edge_add_contract(TARGET_NAME PRODUCT_YAML LOCAL_YAML)
edge_add_contract(
    contract_${PROD_NAME} 
    "${CMAKE_SOURCE_DIR}/contracts/products/dcu.yaml"
    "${CMAKE_CURRENT_SOURCE_DIR}/extension.yaml"
)

add_executable(${PROD_NAME} main.c)

# 输出路径标准化
set_target_properties(${PROD_NAME} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/products/${PROD_NAME}/bin"
)

target_include_directories(${PROD_NAME} PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
)

# 4. 链接依赖
target_link_libraries(${PROD_NAME} PRIVATE 
    edge_algo
    edge_stream_engine
    edge_io_executor
    edge_main_loop
    contract_${PROD_NAME}
    edge_common_interface
    pthread dl rt m
)

# 5. 安装配置
install_edge_product(${PROD_NAME})
