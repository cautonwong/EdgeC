# src/CMakeLists.txt

# --- Core SDK Library ---
add_library(edge_sdk STATIC
    # Core components
    core/edge_version.c
    core/edge_params.c
    core/shmring.c
    core/datahub.c
    core/net_buf.c
    core/sqlite_service.c
    core/persistence_mgr.c
    core/edge_log.c
    core/contract_registry.c

    # Stream Engine
    stream_engine/stream_engine.c
    stream_engine/js_engine.c
    stream_engine/script_loader.c
    stream_engine/aggregator.c
    stream_engine/standard_stages.c
    stream_engine/energy_integrator.c
    stream_engine/stage_dsp_rms.c
    stream_engine/js_stage.c
    stream_engine/pipeline_loader.c
    stream_engine/stream_dump.c

    # Main Loop
    main_loop/arbiter.c
    main_loop/bus_orchestrator.c
    main_loop/dcu_inventory.c
    main_loop/edge_service.c
    main_loop/northbound_mgr.c
    main_loop/policy_syncer.c
    main_loop/rest_server.c
    main_loop/sched_engine.c
    main_loop/sched_port.c
    main_loop/web_server.c
    main_loop/ws_proto.c
    
    # Platform specific
    platform/linux/io_executor_linux.c
)

target_include_directories(edge_sdk PUBLIC 
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/algo/include
)

target_link_libraries(edge_sdk
    PRIVATE
        Threads::Threads
        uv_a
        sqlite3
        z
)
