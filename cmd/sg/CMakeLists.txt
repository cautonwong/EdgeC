# SG EL2025 Product Build Script
# ------------------------------------------------------------------------------

set(PROD_NAME sg_el2025)

# 1. 契约生成 (SG 暂无本地扩展)
edge_add_contract(contract_${PROD_NAME} "${CMAKE_SOURCE_DIR}/contracts/products/sg_el2025.yaml" "${CMAKE_CURRENT_SOURCE_DIR}/extension.yaml")


# 2. 定义执行文件 (源码在 cmd/sg/)
add_executable(${PROD_NAME} main.c)
add_web_product(${PROD_NAME} "${CMAKE_SOURCE_DIR}/web/products/${PROD_NAME}")

# 3. 输出路径标准化
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
