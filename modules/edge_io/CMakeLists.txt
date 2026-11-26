# modules/edge_io/CMakeLists.txt

file(GLOB ENGINE_SOURCES src/engines/*.c)
file(GLOB REQUEST_SOURCES src/requests/*.c)

set(IO_SOURCES
    src/bus_orchestrator.c
    src/guard_port.c
    src/passthrough_mgr.c
    src/protocol_bridge.c
    src/ws_proto.c
    ${ENGINE_SOURCES}
    ${REQUEST_SOURCES}
    # 服务层与调度层代码
    src/sched_engine.c
    src/sched_port.c
)

add_library(edge_io STATIC
    ${IO_SOURCES}
)

target_include_directories(edge_io PUBLIC include src/requests)
target_include_directories(edge_io PRIVATE src src/engines)

# edge_io: 包含 SnF 断网落盘缓存 (snf_manager.c) 与 MQTT 南北向执行器 (edge_agent.c, io_executor_linux.c) 显式适配器
target_link_libraries(edge_io PUBLIC edge_infra edge_core edge_codec edge_session edge_linux_platform edge_common_interface cyclonedds)
target_link_libraries(edge_io PRIVATE libuv::uv_a cjson CURL::libcurl open62541::open62541 m)


