cmake_minimum_required(VERSION 3.10)
project(EdgeC_Tests)

# No need to manual include dirs, edge_core_lib carries PUBLIC include props
# But cmocka is test-only, so we link it here.

# Test Algorithm
add_executable(test_algorithm_sum test_algorithm_sum.c)
target_link_libraries(test_algorithm_sum PRIVATE edge_core_lib cmocka::cmocka pthread dl rt m)
add_test(NAME AlgorithmSum COMMAND test_algorithm_sum)

# Test SQLite
add_executable(test_sqlite_service test_sqlite_service.c)
target_link_libraries(test_sqlite_service PRIVATE edge_core_lib cmocka::cmocka pthread dl rt m)
add_test(NAME SQLiteService COMMAND test_sqlite_service)

# Test Protocol
add_executable(test_modbus_protocol test_modbus_protocol.c)
target_link_libraries(test_modbus_protocol PRIVATE edge_core_lib cmocka::cmocka pthread dl rt m)
target_link_options(test_modbus_protocol PRIVATE -Wl,--wrap=io_submit)
add_test(NAME ModbusProtocol COMMAND test_modbus_protocol)

# Test ShmRing
add_executable(test_shmring test_shmring.c)
target_link_libraries(test_shmring PRIVATE edge_core_lib cmocka::cmocka pthread dl rt m)
add_test(NAME ShmRing COMMAND test_shmring)

# Test Version
add_executable(test_version test_version.c)
target_link_libraries(test_version PRIVATE edge_core_lib cmocka::cmocka pthread dl rt m)
add_test(NAME Version COMMAND test_version)

# Test Arbiter
add_executable(test_arbiter test_arbiter.c)
target_link_libraries(test_arbiter PRIVATE edge_core_lib cmocka::cmocka pthread dl rt m)
add_test(NAME Arbiter COMMAND test_arbiter)

# Test Driver Manager
add_executable(test_driver_manager test_driver_manager.c)
target_link_libraries(test_driver_manager PRIVATE edge_core_lib cmocka::cmocka pthread dl rt m)
add_test(NAME DriverManager COMMAND test_driver_manager)

# Test IO Executor
add_executable(test_io_executor test_io_executor.c)
target_link_libraries(test_io_executor PRIVATE edge_core_lib cmocka::cmocka pthread dl rt m)
add_test(NAME IOExecutor COMMAND test_io_executor)

# Test Integration
add_executable(test_integration test_integration.c)
target_link_libraries(test_integration PRIVATE edge_core_lib cmocka::cmocka pthread dl rt m)
target_link_options(test_integration PRIVATE -Wl,--wrap=io_submit)
add_test(NAME Integration COMMAND test_integration)