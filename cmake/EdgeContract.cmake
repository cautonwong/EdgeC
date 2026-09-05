# EdgeOS Contract Build Module v2.0 (Native Go)
# ------------------------------------------------------------------------------

# @brief 为指定产品生成查表契约库
# 
# @param TARGET_NAME  生成的 CMake 目标库名称 (如 contract_iems)
# @param PRODUCT_YAML 产品核心配置文件
# @param LOCAL_YAML   产品本地扩展配置 (可选)
function(edge_add_contract TARGET_NAME PRODUCT_YAML LOCAL_YAML)

    # 1. 定位架构相关的 Go 二进制路径
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
        set(ARCH "linux-aarch64")
    else()
        set(ARCH "linux-x86_64")
    endif()

    set(COMPILER_BIN "${CMAKE_SOURCE_DIR}/.local/${ARCH}/bin/contract-compiler")
    set(SCHEMA_PATH "${CMAKE_SOURCE_DIR}/contracts/schema/tags.yaml")

    # 彻底隔离：build/products/<product_name>/...
    string(REPLACE "contract_" "" PROD_NAME ${TARGET_NAME})
    set(PROD_DIR "${CMAKE_BINARY_DIR}/products/${PROD_NAME}")
    
    set(GEN_DIR "${PROD_DIR}/generated")
    set(LIB_DIR "${PROD_DIR}/lib")
    
    set(OUT_C "${GEN_DIR}/contracts.c")
    set(OUT_ROUTER_C "${GEN_DIR}/edge_router_table.c")
    set(OUT_SLAVE_C "${GEN_DIR}/contracts_slave.c")
    set(OUT_DRIVER_REG_C "${GEN_DIR}/edge_driver_registry.c")
    set(OUT_DRIVER_REG_H "${GEN_DIR}/edge_driver_registry.h")
    set(OUT_H "${GEN_DIR}/contracts.h")
    set(OUT_YAML "${GEN_DIR}/contracts.yaml")
    set(OUT_CMAKE "${GEN_DIR}/config.cmake")
    set(OUT_JSON "${GEN_DIR}/omnis_manifest.json")
    set(OUT_TS "${GEN_DIR}/tags_type.ts")
    set(OUT_SQL "${GEN_DIR}/init.sql")
    set(OUT_GO "${GEN_DIR}/tags.go")

    # 2. 构建命令参数
    set(CMD_ARGS 
        "--product" "${PRODUCT_YAML}"
        "--out" "${GEN_DIR}"
        "--base" "${CMAKE_SOURCE_DIR}"
    )

    if(NOT "${LOCAL_YAML}" STREQUAL "")
        list(APPEND CMD_ARGS "--local" "${LOCAL_YAML}")
        set(LOCAL_DEP "${LOCAL_YAML}")
    endif()

    # 2. 如果生成文件不存在，在 CMake 配置期预先运行编译器生成
    if(NOT EXISTS "${OUT_C}" OR NOT EXISTS "${OUT_DRIVER_REG_C}" OR NOT EXISTS "${OUT_DRIVER_REG_H}")
        execute_process(
            COMMAND "${COMPILER_BIN}" ${CMD_ARGS}
        )
        execute_process(
            COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/aggregate_go_tags.py"
        )
    endif()

    add_custom_command(
        OUTPUT "${OUT_C}" "${OUT_H}" "${OUT_YAML}" "${OUT_JSON}" "${OUT_TS}" "${OUT_SQL}" "${OUT_SLAVE_C}" "${OUT_ROUTER_C}" "${OUT_DRIVER_REG_C}" "${OUT_DRIVER_REG_H}" "${OUT_GO}"
        COMMAND "${COMPILER_BIN}" ${CMD_ARGS}
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/aggregate_go_tags.py"
        DEPENDS "${COMPILER_BIN}" "${PRODUCT_YAML}" ${LOCAL_DEP}
        COMMENT "[Contract] Generating native lookup tables and Go bindings for ${TARGET_NAME}"
        VERBATIM
    )

    # 4. 创建静态库并精准控制其输出路径
    add_library(${TARGET_NAME} STATIC "${OUT_C}" "${OUT_DRIVER_REG_C}")
    set_target_properties(${TARGET_NAME} PROPERTIES 
        POSITION_INDEPENDENT_CODE ON
    )
    target_link_libraries(${TARGET_NAME} PUBLIC edge_io edge_common_interface)
    target_include_directories(${TARGET_NAME} PUBLIC 
        "${GEN_DIR}"
        "${CMAKE_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/modules/edge_core/include"
        "${CMAKE_SOURCE_DIR}/modules/edge_io/include"
        "${CMAKE_SOURCE_DIR}/algo/include"
        "${CMAKE_SOURCE_DIR}/.local/${SYSTEM_NAME_LOWER}-${CMAKE_HOST_SYSTEM_PROCESSOR}/include"
    )

    message(STATUS "[Contract] Target ${TARGET_NAME} created as STATIC library")

endfunction()
