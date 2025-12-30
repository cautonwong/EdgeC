cmake_minimum_required(VERSION 3.10)
project(EdgeC_Tests)

# This macro simplifies the definition of a test executable.
# It automatically adds the core library sources and links all necessary dependencies.
macro(add_edge_test TEST_NAME)
    add_executable(${TEST_NAME}
        ${ARGN} # Pass extra source files like mock_time.c
        ${EDGE_CORE_LIB_SOURCES}
    )
    target_include_directories(${TEST_NAME} PRIVATE ../include ../algo/include)
    target_link_libraries(${TEST_NAME} PRIVATE
        contracts_iems  # Provide a concrete contract for tests
        cmocka::cmocka
        cJSON::cjson
        libuv::uv
        SQLite::SQLite3
        Modbus::Modbus
        QuickJS::QuickJS
        pthread dl rt m
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endmacro()

# Add all test executables using the new macro
add_edge_test(test_algorithm_sum test_algorithm_sum.c)
add_edge_test(test_battery_algo test_battery_algo.c)
add_edge_test(test_sqlite_service test_sqlite_service.c)
add_edge_test(test_stream_engine test_stream_engine.c)
add_edge_test(test_stream_engine_industrial test_stream_engine_industrial.c)
add_edge_test(test_sdk_full_chain test_sdk_full_chain.c)
add_edge_test(test_stream_engine_timeguard test_stream_engine_timeguard.c)
add_edge_test(test_pipeline_loader test_pipeline_loader.c)
add_edge_test(test_js_engine test_js_engine.c)
add_edge_test(test_script_exec test_script_exec.c)
add_edge_test(test_shmring test_shmring.c)
add_edge_test(test_version test_version.c)
add_edge_test(test_arbiter test_arbiter.c)
add_edge_test(test_driver_manager test_driver_manager.c)
add_edge_test(test_io_executor test_io_executor.c)
add_edge_test(test_net_buf test_net_buf.c)
add_edge_test(test_fuzz_datahub test_fuzz_datahub.c)

# Special test cases with extra files or link options

# test_scheduler is a special case: it provides a mock implementation of time functions.
# We must build it with a custom source list that excludes the real time source file.
set(SCHEDULER_TEST_SOURCES ${EDGE_CORE_LIB_SOURCES})
list(FILTER SCHEDULER_TEST_SOURCES EXCLUDE REGEX ".*/sched_time_source.c$")
add_executable(test_scheduler 
    test_scheduler.c 
    mock_time.c
    ${SCHEDULER_TEST_SOURCES}
)
target_include_directories(test_scheduler PRIVATE ../include ../algo/include)
target_link_libraries(test_scheduler PRIVATE
    contracts_iems
    cmocka::cmocka
    cJSON::cjson
    libuv::uv
    SQLite::SQLite3
    Modbus::Modbus
    QuickJS::QuickJS
    pthread dl rt m
)
add_test(NAME test_scheduler COMMAND test_scheduler)

add_edge_test(test_integration test_integration.c)
target_link_options(test_integration PRIVATE -Wl,--wrap=io_submit)

