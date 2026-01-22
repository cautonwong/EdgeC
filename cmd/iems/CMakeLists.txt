# IEMS Product Build Script
# ------------------------------------------------------------------------------

set(PROD_NAME iems)

# 1. 契约生成 (产品自治：指定自己的清单 YAML)
edge_add_contract(
    contract_${PROD_NAME}
    "${CMAKE_SOURCE_DIR}/contracts/products/iems.yaml"
    "${CMAKE_CURRENT_SOURCE_DIR}/manifest.yaml"
)

# 2. 定义执行文件与 Web UI
add_executable(${PROD_NAME} main.c ../../src/stream_engine/dsp_stages.c)
add_web_product(${PROD_NAME} "${CMAKE_SOURCE_DIR}/web/products/${PROD_NAME}")

# 3. 输出路径标准化
set_target_properties(${PROD_NAME} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/products/${PROD_NAME}/bin"
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

# 6. [NEW] Conditionally Build UI Companion Process
if(ENABLE_UI)
    message(STATUS "[${PROD_NAME}] Building CLI-UI Companion Process...")

    # Define the CLI process target
    add_executable(${PROD_NAME}-cli
        ${CMAKE_SOURCE_DIR}/src/ui/core/cli_main_entry.c # Common UI entry point
        ui_dashboard.c                                  # Product-specific UI pages
    )

    # Link dependencies
    find_package(PahoMQTT REQUIRED) # Ensure MQTT lib is found
    target_link_libraries(${PROD_NAME}-cli PRIVATE
        edge_ui_framework
        contract_${PROD_NAME}
        edge_core
        ncursesw
    )
    
    # Add compile definition for the driver
    target_compile_definitions(${PROD_NAME}-cli PRIVATE UI_DRIVER_NCURSES)

    # [Key Point] Unify the output directory
    set_target_properties(${PROD_NAME}-cli PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/products/${PROD_NAME}/bin"
    )

    # Installation rule
    install(TARGETS ${PROD_NAME}-cli DESTINATION bin)
endif()


