# EdgeOS DCU 编译配置 (自治解耦重构版)

set(PROD_NAME dcu)

# 签名: edge_add_contract(TARGET_NAME PRODUCT_YAML LOCAL_YAML)
edge_add_contract(
    contract_${PROD_NAME} 
    "${CMAKE_SOURCE_DIR}/contracts/products/dcu.yaml"
    "${CMAKE_CURRENT_SOURCE_DIR}/manifest.yaml"
)

# 1. 声明集中器专属业务自治静态库
add_library(dcu_app_lib STATIC
    dcu_inventory.c
    dcu_poller.c
    dcu_storage.c
    dcu_router.c
    dcu_dynamic_poller.c
    dcu_security.c
    dcu_discovery.c
    dcu_event_monitor.c
    dcu_topology_lineloss.c
    dcu_load_control.c
    dcu_outage_detector.c
    dcu_vertical_tasks.c
    dcu_horizontal_tasks.c
    dcu_soe_event_engine.c
    dcu_peripheral_telecontrol.c
    dcu_data_fusion_engine.c
    dcu_dlt698_mgmt_engine.c
    dcu_app_daemon.c
)

# 2. 自治库头文件依赖关系
target_include_directories(dcu_app_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/products/${PROD_NAME}/generated
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/modules/edge_io/include
    ${CMAKE_SOURCE_DIR}/modules/edge_stream/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
    ${CMAKE_SOURCE_DIR}/modules/edge_protocol/include
    ${CMAKE_SOURCE_DIR}/modules/edge_security/include
)

# 3. 自治库链接外部依赖 (包含 DCU 本地微数据库引擎 dcu_storage/dcu_inventory 适配器)
target_link_libraries(dcu_app_lib PUBLIC
    edge_infra
    edge_io
    edge_stream
    edge_security
    edge_core
    edge_protocol
    edge_algo
    SQLite::SQLite3
    libuv::uv_a
    contract_${PROD_NAME}
)
add_dependencies(dcu_app_lib contract_${PROD_NAME})

# 4. 创建最终可执行应用程序
add_executable(${PROD_NAME} main.c)

# 输出路径标准化
set_target_properties(${PROD_NAME} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/products/${PROD_NAME}/bin"
)

target_include_directories(${PROD_NAME} PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
)

# 5. 链接自治库与基础运行时库
target_link_libraries(${PROD_NAME} PRIVATE 
    -Wl,--start-group
    dcu_app_lib
    -Wl,--end-group
    contract_${PROD_NAME}
    edge_common_interface
    pthread dl rt m
)

# 6. 安装配置
install_edge_product(${PROD_NAME})

