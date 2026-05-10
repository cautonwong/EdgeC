# EdgeOS Contract Compiler Build Module
# ------------------------------------------------------------------------------

find_program(GO_EXECUTABLE go)

if(NOT GO_EXECUTABLE)
    message(FATAL_ERROR "Go compiler not found! Please install Go to build the contract compiler.")
endif()

# @brief 构建 Go 版契约编译器
function(build_contract_compiler)
    set(COMPILER_SRC_DIR "${CMAKE_SOURCE_DIR}/tools/contract-compiler-v2")
    
    # 确定架构路径
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
        set(ARCH "linux-aarch64")
    else()
        set(ARCH "linux-x86_64")
    endif()

    set(COMPILER_BIN_OUTPUT "${CMAKE_SOURCE_DIR}/.local/${ARCH}/bin/contract-compiler")

    message(STATUS "[Compiler] Setting up build for contract-compiler-v2...")

    # 使用 add_custom_command 来追踪 Go 源码变化
    file(GLOB_RECURSE GO_SOURCES "${COMPILER_SRC_DIR}/*.go")

    add_custom_command(
        OUTPUT "${COMPILER_BIN_OUTPUT}"
        COMMAND ${GO_EXECUTABLE} build -o "${COMPILER_BIN_OUTPUT}" cmd/main.go
        WORKING_DIRECTORY "${COMPILER_SRC_DIR}"
        DEPENDS ${GO_SOURCES} "${COMPILER_SRC_DIR}/go.mod"
        COMMENT "[Go] Building contract-compiler-v2 binary..."
        VERBATIM
    )

    # 创建一个全局目标，确保其他模块可以依赖它
    add_custom_target(contract_compiler_bin ALL DEPENDS "${COMPILER_BIN_OUTPUT}")

endfunction()
