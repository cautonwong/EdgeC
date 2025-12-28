cmake_minimum_required(VERSION 3.10)
project(EdgeC_Tests)

# Target dependencies are already defined in GLOBAL scope by root CMakeLists.txt

# Test Algorithm
add_executable(test_algorithm_sum test_algorithm_sum.c)
target_link_libraries(test_algorithm_sum PRIVATE edge_core_lib cmocka::cmocka)
add_test(NAME AlgorithmSum COMMAND test_algorithm_sum)

add_executable(test_battery_algo test_battery_algo.c)
target_link_libraries(test_battery_algo PRIVATE edge_core_lib cmocka::cmocka)
add_test(NAME BatteryAlgo COMMAND test_battery_algo)

# Test SQLite Service (The new one)
add_executable(test_sqlite_service test_sqlite_service.c)
target_link_libraries(test_sqlite_service PRIVATE edge_core_lib cmocka::cmocka)
add_test(NAME SQLiteService COMMAND test_sqlite_service)

# Test JS Engine
add_executable(test_js_engine test_js_engine.c)
target_link_libraries(test_js_engine PRIVATE edge_core_lib cmocka::cmocka)
add_test(NAME JSEngine COMMAND test_js_engine)

# Test Script Executor
add_executable(test_script_exec test_script_exec.c)
target_link_libraries(test_script_exec PRIVATE edge_core_lib cmocka::cmocka)
add_test(NAME ScriptExec COMMAND test_script_exec)

# Test ShmRing
add_executable(test_shmring test_shmring.c)
target_link_libraries(test_shmring PRIVATE edge_core_lib cmocka::cmocka)
add_test(NAME ShmRing COMMAND test_shmring)

# Test Version
add_executable(test_version test_version.c)
target_link_libraries(test_version PRIVATE edge_core_lib cmocka::cmocka)
add_test(NAME Version COMMAND test_version)

# Test Arbiter
add_executable(test_arbiter test_arbiter.c)
target_link_libraries(test_arbiter PRIVATE edge_core_lib cmocka::cmocka)
add_test(NAME Arbiter COMMAND test_arbiter)

# Test Driver Manager
add_executable(test_driver_manager test_driver_manager.c)
target_link_libraries(test_driver_manager PRIVATE edge_core_lib cmocka::cmocka)
add_test(NAME DriverManager COMMAND test_driver_manager)

# Test IO Executor
add_executable(test_io_executor test_io_executor.c)
target_link_libraries(test_io_executor PRIVATE edge_core_lib cmocka::cmocka)
add_test(NAME IOExecutor COMMAND test_io_executor)

# Test NetBuf
add_executable(test_net_buf test_net_buf.c)
target_link_libraries(test_net_buf PRIVATE edge_core_lib cmocka::cmocka)
add_test(NAME NetBuf COMMAND test_net_buf)

# Test Integration
add_executable(test_integration test_integration.c)
target_link_libraries(test_integration PRIVATE edge_core_lib cmocka::cmocka)
target_link_options(test_integration PRIVATE -Wl,--wrap=io_submit)
add_test(NAME Integration COMMAND test_integration)

# Test Scheduler
add_executable(test_scheduler test_scheduler.c ../tests/mock_time.c)
target_include_directories(test_scheduler PRIVATE ../include)
target_link_libraries(test_scheduler PRIVATE edge_core_lib cmocka::cmocka libuv::uv)
add_test(NAME Scheduler COMMAND test_scheduler)

# Test Fuzz DataHub
add_executable(test_fuzz_datahub test_fuzz_datahub.c)
target_link_libraries(test_fuzz_datahub PRIVATE edge_core_lib cmocka::cmocka)
add_test(NAME FuzzDataHub COMMAND test_fuzz_datahub)
