# Define the test executable
add_executable(unit_tests test_version.c)
# Link the core library and cmocka
target_link_libraries(unit_tests PRIVATE edge_core_lib ${CMOCKA_LIB})
# Register with CTest
add_test(NAME VersionCheck COMMAND unit_tests)

# Arbiter test executable
add_executable(arbiter_test test_arbiter.c)
target_link_libraries(arbiter_test PRIVATE edge_core_lib ${UV_LIB} ${CMOCKA_LIB})
add_test(NAME ArbiterTest COMMAND arbiter_test)

# IO Executor test executable
add_executable(io_executor_test test_io_executor.c)
target_link_libraries(io_executor_test PRIVATE edge_core_lib ${UV_LIB} ${CMOCKA_LIB})
add_test(NAME IOExecutorTest COMMAND io_executor_test)

# Algorithm Sum test executable
add_executable(algo_sum_test test_algorithm_sum.c)
target_link_libraries(algo_sum_test PRIVATE edge_core_lib ${UV_LIB} ${CMOCKA_LIB})
add_test(NAME AlgoSumTest COMMAND algo_sum_test)

# SQLite Service test executable
add_executable(sqlite_service_test test_sqlite_service.c)
target_link_libraries(sqlite_service_test PRIVATE edge_core_lib ${UV_LIB} ${CMOCKA_LIB} ${SQLITE3_LIB} m)
add_test(NAME SqliteServiceTest COMMAND sqlite_service_test)

# Modbus Protocol test executable
add_executable(modbus_protocol_test test_modbus_protocol.c)
target_link_libraries(modbus_protocol_test PRIVATE edge_core_lib ${CMOCKA_LIB})
add_test(NAME ModbusProtocolTest COMMAND modbus_protocol_test)

# Integration test executable
add_executable(integration_test test_integration.c)
target_link_libraries(integration_test PRIVATE edge_core_lib ${UV_LIB} ${CMOCKA_LIB})
target_link_options(integration_test PRIVATE "-Wl,--wrap=io_submit")
add_test(NAME IntegrationTest COMMAND integration_test)

# Driver Manager test executable
add_executable(driver_manager_test test_driver_manager.c)
target_link_libraries(driver_manager_test PRIVATE edge_core_lib ${CMOCKA_LIB})
add_test(NAME DriverManagerTest COMMAND driver_manager_test)
