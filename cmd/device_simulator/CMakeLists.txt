# Device Simulator Build Script
set(SIM_NAME device_simulator)

add_executable(${SIM_NAME} main.c)

# 核心：链接规约库，这样模拟器就能生成真实合法的 Modbus/DLMS 报文
target_link_libraries(${SIM_NAME} PRIVATE 
    edge_protocols
    edge_platform
    edge_core
    edge_common_interface
    libuv::uv
    pthread dl rt m
)

set_target_properties(${SIM_NAME} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/products/device_simulator/bin"
)