# EdgeOS Contract Build Module v2.0
# ------------------------------------------------------------------------------

find_package(Python3 REQUIRED)

# @brief 为指定产品生成查表契约库
#
# @param TARGET_NAME  生成的 CMake 目标库名称 (如 contract_iems)
# @param PRODUCT_YAML 产品核心配置文件
# @param LOCAL_YAML   产品本地扩展配置 (可选)
function(edge_add_contract TARGET_NAME PRODUCT_YAML LOCAL_YAML)

    set(TOOL_PATH "${CMAKE_SOURCE_DIR}/tools/contract_compiler.py")
    set(SCHEMA_PATH "${CMAKE_SOURCE_DIR}/contracts/schema/tags.yaml")

    # 彻底隔离：build/products/<product_name>/...
    # 注意：TARGET_NAME 通常是 contract_<name>
    string(REPLACE "contract_" "" PROD_NAME ${TARGET_NAME})
    set(PROD_DIR "${CMAKE_BINARY_DIR}/products/${PROD_NAME}")

    set(GEN_DIR "${PROD_DIR}/generated")
    set(LIB_DIR "${PROD_DIR}/lib")

    set(OUT_C "${GEN_DIR}/contracts.c")
    set(OUT_H "${GEN_DIR}/contracts.h")
    set(OUT_YAML "${GEN_DIR}/contracts.yaml")

    set(CMD_ARGS
        "${Python3_EXECUTABLE}" "${TOOL_PATH}"
        "--schema" "${SCHEMA_PATH}"
        "--product" "${PRODUCT_YAML}"
        "--out" "${GEN_DIR}"
    )

    if(NOT "${LOCAL_YAML}" STREQUAL "")
        list(APPEND CMD_ARGS "--local" "${LOCAL_YAML}")
        set(LOCAL_DEP "${LOCAL_YAML}")
    endif()

    # 定义自定义生成命令 (包含 YAML 产物)
    add_custom_command(
        OUTPUT "${OUT_C}" "${OUT_H}" "${OUT_YAML}"
        COMMAND ${CMD_ARGS}
        DEPENDS "${TOOL_PATH}" "${SCHEMA_PATH}" "${PRODUCT_YAML}" ${LOCAL_DEP}
        COMMENT "[Contract] Generating static lookup tables and audit YAML for ${TARGET_NAME}"
        VERBATIM
    )

    # 创建静态库并精准控制其输出路径
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
