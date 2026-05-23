# modules/edge_io/CMakeLists.txt

file(GLOB ENGINE_SOURCES src/engines/*.c)
file(GLOB REQUEST_SOURCES src/requests/*.c)
file(GLOB DRIVER_SOURCES src/driver_*.c)

add_library(edge_io STATIC
    src/bus_orchestrator.c
    src/driver_manager.c
    src/passthrough_mgr.c
    src/dcu_dynamic_poller.c
    src/dcu_inventory.c
    src/dcu_poller.c
    src/dcu_storage.c
    src/protocol_bridge.c
    src/cloud_sync_service.c
    src/rest_server.c
    src/web_server.c
    src/ws_proto.c
    src/persistence_mgr.c
    src/snf_manager.c
    src/profile_loader.c
    src/session_manager.c
    # 命名不符合 driver_*.c 模式，需要显式列出 (以接地气的中文添加注释)
    src/edge_api.c
    src/adc_driver.c
    src/modbus_rtu_driver.c
    src/modbus_tcp_driver.c
    ${ENGINE_SOURCES}
    ${REQUEST_SOURCES}
    ${DRIVER_SOURCES}
    # 从旧 src/ 目录引入尚未迁移至模块的服务层代码
    ${CMAKE_SOURCE_DIR}/src/main_loop/northbound_mgr.c
    ${CMAKE_SOURCE_DIR}/src/main_loop/sched_engine.c
    ${CMAKE_SOURCE_DIR}/src/main_loop/sched_port.c
    ${CMAKE_SOURCE_DIR}/src/platform/linux/io_executor_linux.c
)

target_include_directories(edge_io PUBLIC include src/requests)
target_include_directories(edge_io PRIVATE src/engines)
target_link_libraries(edge_io PUBLIC edge_core edge_protocol edge_stream edge_security)
target_link_libraries(edge_io PRIVATE libuv::uv_a SQLite::SQLite3 cjson PahoMQTT::PahoMQTT CURL::libcurl)

