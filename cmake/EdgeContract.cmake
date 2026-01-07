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
    set(OUT_TS "${GEN_DIR}/tags_type.ts")

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

    # 定义自定义生成命令 (仅在构建目录生成)
    add_custom_command(
        OUTPUT "${OUT_C}" "${OUT_H}" "${OUT_YAML}" "${OUT_CMAKE}" "${OUT_TS}"
        COMMAND "${COMPILER_BIN}" ${CMD_ARGS}
        DEPENDS "${COMPILER_BIN}" "${SCHEMA_PATH}" "${PRODUCT_YAML}" ${LOCAL_DEP}
        COMMENT "[Contract] Generating native lookup tables and TS types for ${TARGET_NAME}"
        VERBATIM
    )

    # 3. 如果文件已生成（通常是第二次配置或已经构建过），则引入 CMake 配置
    # 这允许格在 CMake 中使用 if(CONFIG_XXX) 等逻辑
    if(EXISTS "${OUT_CMAKE}")
        include("${OUT_CMAKE}")
        message(STATUS "[Contract] ${TARGET_NAME} configuration loaded from ${OUT_CMAKE}")
    endif()

    # 4. 创建静态库并精准控制其输出路径
    add_library(${TARGET_NAME}_obj OBJECT "${OUT_C}")
    set_target_properties(${TARGET_NAME}_obj PROPERTIES 
        POSITION_INDEPENDENT_CODE ON
    )
    target_include_directories(${TARGET_NAME}_obj PRIVATE 
        "${GEN_DIR}"
        "${CMAKE_SOURCE_DIR}/include"
    )

    # 创建一个 INTERFACE 库供外部链接，自动传播头文件路径
    add_library(${TARGET_NAME} INTERFACE)
    target_sources(${TARGET_NAME} INTERFACE $<TARGET_OBJECTS:${TARGET_NAME}_obj>)
    target_include_directories(${TARGET_NAME} INTERFACE 
        "${GEN_DIR}"
        "${CMAKE_SOURCE_DIR}/include"
    )

    # 如果有对应的 lib 需求，也可以保留静态库链接逻辑，但推荐使用 INTERFACE 传播
    message(STATUS "[Contract] Target ${TARGET_NAME} created as INTERFACE with OBJECT sources")

endfunction()
