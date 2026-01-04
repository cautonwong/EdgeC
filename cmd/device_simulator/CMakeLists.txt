# Device Simulator Product Build Script
# ------------------------------------------------------------------------------

set(PROD_NAME device_simulator)

# 1. 契约生成
edge_add_contract(
    contract_${PROD_NAME}
    "${CMAKE_SOURCE_DIR}/contracts/products/device_simulator.yaml"
    ""
)

# 2. 定义执行文件
add_executable(${PROD_NAME} main.c)
add_dependencies(${PROD_NAME} contract_${PROD_NAME})

# 3. 输出路径标准化
set_target_properties(${PROD_NAME} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/products/${PROD_NAME}/bin"
)

# 4. 链接依赖 - 引入 IO 执行器以支持高质量规约仿真
target_link_libraries(${PROD_NAME} PRIVATE 
    -Wl,--start-group
    edge_io_executor
    edge_core
    contract_${PROD_NAME}
    -Wl,--end-group
    uv pthread dl rt m
)

# 5. 安装
install_edge_product(${PROD_NAME})
