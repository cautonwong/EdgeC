# EdgeOS VPP Demo Build Script v2.0
# ------------------------------------------------------------------------------

set(PROD_NAME vpp_demo)

# 1. 契约生成
edge_add_contract(
    contract_${PROD_NAME}
    "${CMAKE_CURRENT_SOURCE_DIR}/vpp_product.yaml"
    ""
)

# 2. 定义执行文件
add_executable(${PROD_NAME} main.c)

# 3. 输出重定向 (沙箱化)
set_target_properties(${PROD_NAME} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/products/${PROD_NAME}/bin"
)

# 4. 链接依赖
target_link_libraries(${PROD_NAME} PRIVATE 
    edge_main_loop
    edge_stream_engine
    edge_io_executor
    edge_algo
    contract_${PROD_NAME}
    pthread dl rt m
)