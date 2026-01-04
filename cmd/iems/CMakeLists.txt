# IEMS Product Build Script
# ------------------------------------------------------------------------------

set(PROD_NAME iems)

# 1. 契约生成 (产品自治：指定自己的 YAML 和扩展)
edge_add_contract(
    contract_${PROD_NAME}
    "${CMAKE_SOURCE_DIR}/contracts/products/iems.yaml"
    "${CMAKE_CURRENT_SOURCE_DIR}/extension.yaml"
)

# 2. 定义执行文件与 Web UI
add_executable(${PROD_NAME} main.c)
add_web_product(${PROD_NAME} "${CMAKE_SOURCE_DIR}/web/products/${PROD_NAME}")

# 3. 输出路径标准化
set_target_properties(${PROD_NAME} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/products/${PROD_NAME}/bin"
)

# 4. 链接依赖
target_link_libraries(${PROD_NAME} PRIVATE 
    -Wl,--start-group
    edge_main_loop
    edge_stream_engine
    edge_io_executor
    edge_algo
    contract_${PROD_NAME}
    -Wl,--end-group
    pthread dl rt m
)

# 5. 安装配置
install_edge_product(${PROD_NAME})


