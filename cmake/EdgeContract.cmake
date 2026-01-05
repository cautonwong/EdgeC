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
    set(OUT_SLAVE_C "${GEN_DIR}/contracts_slave.c")
    set(OUT_H "${GEN_DIR}/contracts.h")
    set(OUT_YAML "${GEN_DIR}/contracts.yaml")
    set(OUT_CMAKE "${GEN_DIR}/config.cmake")

    # 2. 构建命令参数
    set(CMD_ARGS 
        "--schema" "${SCHEMA_PATH}"
        "--product" "${PRODUCT_YAML}"
        "--out" "${GEN_DIR}"
        "--base" "${CMAKE_SOURCE_DIR}"
    )

    if(NOT "${LOCAL_YAML}" STREQUAL "")
        list(APPEND CMD_ARGS "--local" "${LOCAL_YAML}")
        set(LOCAL_DEP "${LOCAL_YAML}")
    endif()

    # 定义自定义生成命令 (包含 YAML 和 CMake 产物)
    add_custom_command(
        OUTPUT "${OUT_C}" "${OUT_H}" "${OUT_YAML}" "${OUT_CMAKE}"
        COMMAND "${COMPILER_BIN}" ${CMD_ARGS}
        DEPENDS "${COMPILER_BIN}" "${SCHEMA_PATH}" "${PRODUCT_YAML}" ${LOCAL_DEP}
        COMMENT "[Contract] Generating native lookup tables and config for ${TARGET_NAME}"
        VERBATIM
    )

    # 3. 如果文件已生成（通常是第二次配置或已经构建过），则引入 CMake 配置
    # 这允许格在 CMake 中使用 if(CONFIG_XXX) 等逻辑
    if(EXISTS "${OUT_CMAKE}")
        include("${OUT_CMAKE}")
        message(STATUS "[Contract] ${TARGET_NAME} configuration loaded from ${OUT_CMAKE}")
    endif()

    # 4. 创建静态库并精准控制其输出路径
    add_library(${TARGET_NAME} STATIC "${OUT_C}")
    set_target_properties(${TARGET_NAME} PROPERTIES 
        ARCHIVE_OUTPUT_DIRECTORY "${LIB_DIR}"
    )
    
    target_include_directories(${TARGET_NAME} 
        PUBLIC 
        "${GEN_DIR}"
        "${CMAKE_SOURCE_DIR}/include"
    )

endfunction()
