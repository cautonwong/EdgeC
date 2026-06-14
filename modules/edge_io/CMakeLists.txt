# modules/edge_io/CMakeLists.txt

file(GLOB ENGINE_SOURCES src/engines/*.c)
file(GLOB REQUEST_SOURCES src/requests/*.c)
file(GLOB DRIVER_SOURCES src/driver_*.c)

set(IO_SOURCES
    src/bus_orchestrator.c
    src/driver_manager.c
    src/passthrough_mgr.c
    src/protocol_bridge.c
    src/cloud_sync_service.c
    src/rest_server.c
    src/web_server.c
    src/ws_proto.c
    src/profile_loader.c
    src/session_manager.c
    # 命名不符合 driver_*.c 模式，需要显式列出 (以接地气的中文添加注释)
    src/edge_api.c
    src/adc_driver.c
    src/modbus_rtu_driver.c
    src/modbus_tcp_driver.c
    src/driver_dlt698.c
    ${CMAKE_SOURCE_DIR}/third-party/open62541/open62541.c
    ${ENGINE_SOURCES}
    ${REQUEST_SOURCES}
    ${DRIVER_SOURCES}
    # 从旧 src/ 目录引入尚未迁移至模块的服务层代码
    ${CMAKE_SOURCE_DIR}/src/main_loop/northbound_mgr.c
    ${CMAKE_SOURCE_DIR}/src/main_loop/sched_engine.c
    ${CMAKE_SOURCE_DIR}/src/main_loop/sched_port.c
    ${CMAKE_SOURCE_DIR}/src/platform/linux/io_executor_linux.c
)

# 根据 Kconfig 条件加载 Store-and-Forward (SnF) 断线缓存与可靠传输机制 (以接地气的中文添加注释)
if(CONFIG_EDGEC_SNF)
    list(APPEND IO_SOURCES src/snf_manager.c)
endif()

# 根据 Kconfig 条件加载 Periodic Persistence 定时周期性持久化同步服务
if(CONFIG_EDGEC_PERSISTENCE)
    list(APPEND IO_SOURCES src/persistence_mgr.c)
endif()

add_library(edge_io STATIC
    ${IO_SOURCES}
)

target_include_directories(edge_io PUBLIC include src/requests ${CMAKE_SOURCE_DIR}/third-party/open62541)
target_include_directories(edge_io PRIVATE src/engines)
target_link_libraries(edge_io PUBLIC edge_core edge_protocol edge_stream edge_security)
target_link_libraries(edge_io PRIVATE libuv::uv_a SQLite::SQLite3 cjson PahoMQTT::PahoMQTT CURL::libcurl)


