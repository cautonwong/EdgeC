# ZephyrBuild.cmake
# 专门用于处理跨架构的 Zephyr 固件构建

include(ExternalProject)

# 默认参数
set(ZEPHYR_BOARD "qemu_cortex_m3" CACHE STRING "Zephyr target board")

function(add_zephyr_app APP_NAME APP_PATH)
    get_filename_component(ABS_APP_PATH "${APP_PATH}" ABSOLUTE)
    set(APP_BINARY_DIR "${CMAKE_BINARY_DIR}/zephyr/${APP_NAME}")
    
    # 彻底改用 west build 模式，因为 Zephyr 强依赖 west 环境
    ExternalProject_Add(${APP_NAME}_zephyr_build
        SOURCE_DIR "${ABS_APP_PATH}"
        BINARY_DIR "${APP_BINARY_DIR}"
        CONFIGURE_COMMAND ${Python3_EXECUTABLE} -m west build 
            -b ${ZEPHYR_BOARD} 
            -d "${APP_BINARY_DIR}"
            "${ABS_APP_PATH}" 
            --cmake-only
        BUILD_COMMAND ${Python3_EXECUTABLE} -m west build -d "${APP_BINARY_DIR}"
        INSTALL_COMMAND ""
        BUILD_ALWAYS ON
    )
    
    message(STATUS "[Zephyr] Registered app ${APP_NAME} for board ${ZEPHYR_BOARD} via west")
endfunction()
