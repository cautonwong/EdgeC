# EdgeOS DCU 编译配置 (修正参数顺序版)

# 签名: edge_add_contract(TARGET_NAME PRODUCT_YAML LOCAL_YAML)
edge_add_contract(
    contract_dcu 
    "${CMAKE_SOURCE_DIR}/contracts/products/dcu.yaml"
    ""
)

add_executable(dcu main.c)

# 输出路径标准化
set_target_properties(dcu PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/products/dcu/bin"
)

target_include_directories(dcu PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(dcu PRIVATE
    edge_main_loop
    edge_io_executor
    edge_core
    contract_dcu
    pthread dl rt m
)

# 安装路径
install(TARGETS dcu DESTINATION products/dcu/bin)
