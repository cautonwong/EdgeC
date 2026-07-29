cmake_minimum_required(VERSION 3.10)
project(EdgeC_Tests)

# 统一测试构建宏
macro(add_edge_test TEST_NAME)
    add_executable(${TEST_NAME} ${ARGN})
    target_link_libraries(${TEST_NAME} PRIVATE
        -Wl,--start-group
        edge_io
        edge_stream
        edge_security
        edge_core
        edge_protocol
        edge_algo
        -Wl,--end-group
        contract_iems
        edge_common_interface
        cmocka::cmocka
        pthread dl rt m
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endmacro()

# 1. 核心框架测试 (Framework)
add_edge_test(test_version test_version.c)
add_edge_test(test_is_enabled test_is_enabled.c)
add_edge_test(test_edge_log test_edge_log.c)
add_edge_test(test_net_buf test_net_buf.c)
add_edge_test(test_shmring test_shmring.c)
add_edge_test(test_edge_tlv test_edge_tlv.c)
add_edge_test(test_arbiter test_arbiter.c)
add_edge_test(test_dlt698_axdr test_dlt698_axdr.c)
add_edge_test(test_dcu_inventory test_dcu_inventory.c)
add_edge_test(test_dcu_polling_loop test_dcu_polling_loop.c)
add_edge_test(test_dcu_storage test_dcu_storage.c)
add_edge_test(test_dcu_integration test_dcu_integration.c)
add_edge_test(test_dcu_router test_dcu_router.c)
add_edge_test(test_dcu_advanced_modules test_dcu_advanced_modules.c)





# 2. 数据总线测试 (Data Plane)
add_edge_test(test_datahub test_datahub.c)
add_edge_test(test_datahub_shadow test_datahub_shadow.c)
add_edge_test(test_passthrough test_passthrough.c)
add_edge_test(test_memfd_seals test_memfd_seals.c)
add_edge_test(test_fuzz_datahub test_fuzz_datahub.c)
add_edge_test(test_sqlite_service test_sqlite_service.c)
add_edge_test(test_sqlite_app_registry test_sqlite_app_registry.c)
add_edge_test(test_edge_app_mgr test_edge_app_mgr.c)
add_edge_test(test_datahub_dynamic_chunk test_datahub_dynamic_chunk.c)
add_edge_test(test_stream_dag test_stream_dag.c)
add_edge_test(test_edge_mesh test_edge_mesh.c)
add_edge_test(test_fuzz_iec104 test_fuzz_iec104.c)
	add_edge_test(test_fuzz_dlms_dlt698 test_fuzz_dlms_dlt698.c)
add_edge_test(test_dlms_security_validation test_dlms_security_validation.c)

# 3. 执行引擎测试 (Execution Plane)
add_edge_test(test_driver_manager test_driver_manager.c)
add_edge_test(test_io_executor test_io_executor.c)
add_edge_test(test_contract_lookup test_contract_lookup.c)
add_edge_test(test_driver_dlt645 test_driver_dlt645.c)
add_edge_test(test_driver_v3 test_driver_v3.c)
add_edge_test(test_modbus_v3 test_modbus_v3.c)
add_edge_test(test_fuzz_modbus test_fuzz_modbus.c)
add_edge_test(test_radar_fmcw test_radar_fmcw.c)
add_edge_test(test_io_http test_io_http.c)
target_link_libraries(test_io_http PRIVATE CURL::libcurl)
add_edge_test(test_rest_apps test_rest_apps.c)

# 4. 流处理引擎测试 (Compute Plane)
add_edge_test(test_stream_engine test_stream_engine.c)
add_edge_test(test_pipeline_loader test_pipeline_loader.c)
add_edge_test(test_pipeline_hot_reload test_pipeline_hot_reload.c)
add_edge_test(test_js_engine test_js_engine.c)
add_edge_test(test_wasm_stage test_wasm_stage.c)
add_edge_test(test_wasm_host_api test_wasm_host_api.c)
add_edge_test(test_wasm_scheduler test_wasm_scheduler.c)
add_edge_test(test_wasm_sandbox test_wasm_sandbox.c)
add_edge_test(test_sqlite_injection test_sqlite_injection.c)

# 5. 标准算子与算法测试 (Stages & Algos)
add_edge_test(test_linalg test_linalg.c)
add_edge_test(test_algorithm_sum test_algorithm_sum.c)
add_edge_test(test_battery_algo test_battery_algo.c)
add_edge_test(test_ekf_soc test_ekf_soc.c)
add_edge_test(test_mpc_dispatcher test_mpc_dispatcher.c)
add_edge_test(test_sparkplug_b test_sparkplug_b.c)
add_edge_test(test_cia402 test_cia402.c)
add_edge_test(test_foc_latency test_foc_latency.c)
add_edge_test(test_wavelet_anomaly test_wavelet_anomaly.c)
add_edge_test(test_kinematics test_kinematics.c)
add_edge_test(test_dcu_poller test_dcu_poller.c)
add_edge_test(test_protocol_bridge test_protocol_bridge.c)
add_edge_test(test_wasm_secure test_wasm_secure.c)
add_edge_test(test_sensor_fusion test_sensor_fusion.c)
add_edge_test(test_tinyml_overhead test_tinyml_overhead.c)
add_edge_test(test_notch_filter test_notch_filter.c)
add_edge_test(test_ecg_anomaly test_ecg_anomaly.c)
add_edge_test(test_vibration_features test_vibration_features.c)
add_edge_test(test_vibra_sense_rul test_vibra_sense_rul.c)
add_edge_test(test_radar_tracker test_radar_tracker.c)
add_edge_test(test_ahrs_quaternion test_ahrs_quaternion.c)
add_edge_test(test_os_port test_os_port.c)
add_edge_test(test_license_engine test_license_engine.c)
add_edge_test(test_ws_binary test_ws_binary.c)
add_edge_test(test_edge_guard test_edge_guard.c)
add_edge_test(test_snf_resilience test_snf_resilience.c)
add_edge_test(test_ccsds_driver test_ccsds_driver.c)
add_edge_test(test_hl7_fhir test_hl7_fhir.c)
add_edge_test(test_pid_antiwindup test_pid_antiwindup.c)
add_edge_test(test_standard_stages_math test_standard_stages_math.c)
add_edge_test(test_stream_engine_industrial test_stream_engine_industrial.c)
add_edge_test(test_stream_engine_timeguard test_stream_engine_timeguard.c)
add_edge_test(test_proc_filter_pid test_proc_filter_pid.c)
add_edge_test(test_smart_box_hw test_smart_box_hw.c)

# 6. 系统集成测试 (Integration)
add_edge_test(test_integration test_integration.c)
add_edge_test(test_sdk_full_chain test_sdk_full_chain.c)
add_edge_test(test_bus_orchestrator test_bus_orchestrator.c)
add_edge_test(test_omnis_integration test_omnis_integration.c)
add_edge_test(test_dcu test_dcu.c)

# 特殊用例: test_scheduler (Mock Time)
add_executable(test_scheduler test_scheduler.c mock_time.c)
target_link_libraries(test_scheduler PRIVATE
    -Wl,--start-group
    edge_io
    edge_stream
    edge_security
    edge_core
    edge_protocol
    edge_algo
    -Wl,--end-group
    contract_iems
    edge_common_interface
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_scheduler COMMAND test_scheduler)

# 特殊用例: test_northbound_mqtt (Mock MQTT)
add_executable(test_northbound_mqtt 
    test_northbound_mqtt.c
    mock_mqtt.c
    mock_servers.c
)
target_link_libraries(test_northbound_mqtt PRIVATE 
    -Wl,--start-group
    edge_io
    edge_stream
    edge_security
    edge_core
    edge_protocol
    edge_algo
    -Wl,--end-group
    contract_iems
    edge_common_interface
    cmocka::cmocka
    pthread dl rt m
)
target_link_options(test_northbound_mqtt PRIVATE "-Wl,--allow-multiple-definition")
add_test(NAME test_northbound_mqtt COMMAND test_northbound_mqtt)

add_edge_test(test_iems_full_stack test_iems_full_stack.c)

# 编译选项加固
target_link_options(test_integration PRIVATE -Wl,--wrap=io_submit)
add_edge_test(test_adc_dsp_local test_adc_dsp_local.c)

# 追加链接集中器自治库，以彻底消除 undefined reference to dcu_*
target_link_libraries(test_dcu_inventory PRIVATE dcu_app_lib)
target_link_libraries(test_dcu_polling_loop PRIVATE dcu_app_lib)
target_link_libraries(test_dcu_storage PRIVATE dcu_app_lib)
target_link_libraries(test_dcu_integration PRIVATE dcu_app_lib)
target_link_libraries(test_dcu_router PRIVATE dcu_app_lib)
target_link_libraries(test_dcu_poller PRIVATE dcu_app_lib)
target_link_libraries(test_dcu PRIVATE dcu_app_lib)
target_link_libraries(test_dcu_advanced_modules PRIVATE dcu_app_lib)

add_edge_test(test_spsc_ring test_spsc_ring.c)
add_edge_test(test_fall_detection test_fall_detection.c)
add_edge_test(test_sos_dscp test_sos_dscp.c)
add_edge_test(test_wasm_watchdog test_wasm_watchdog.c)
add_edge_test(test_emg_dsp test_emg_dsp.c)
add_edge_test(test_timesync test_timesync.c)
add_edge_test(test_protocol_converter test_protocol_converter.c)
add_edge_test(test_edge_acl test_edge_acl.c)
add_edge_test(test_rehab_kinematics test_rehab_kinematics.c)
add_edge_test(test_motion_wavelet test_motion_wavelet.c)
add_edge_test(test_hrv_analyzer test_hrv_analyzer.c)
add_edge_test(test_ppg_spo2 test_ppg_spo2.c)
add_edge_test(test_bia_estimator test_bia_estimator.c)
add_edge_test(test_ecg_arrhythmia test_ecg_arrhythmia.c)
add_edge_test(test_tremor_analyzer test_tremor_analyzer.c)
add_edge_test(test_gait_balance test_gait_balance.c)
add_edge_test(test_apnea_detector test_apnea_detector.c)
add_edge_test(test_dicom_parser test_dicom_parser.c)
add_edge_test(test_federated_aggregator test_federated_aggregator.c)
add_edge_test(test_shm_waitset test_shm_waitset.c)
add_edge_test(test_thermal_throttle test_thermal_throttle.c)
add_edge_test(test_device_twin test_device_twin.c)
add_edge_test(test_resilient_buffer test_resilient_buffer.c)
add_edge_test(test_driver_hotplug test_driver_hotplug.c)
add_edge_test(test_dfu_dualbank test_dfu_dualbank.c)
add_edge_test(test_impedance_control test_impedance_control.c)
add_edge_test(test_gait_safety test_gait_safety.c)
add_edge_test(test_facility_coordinator test_facility_coordinator.c)
add_edge_test(test_ebpf_guard test_ebpf_guard.c)
add_edge_test(test_simd_cdd_codec test_simd_cdd_codec.c)
add_edge_test(test_multibus_bridge test_multibus_bridge.c)
add_edge_test(test_stream_sql_dag test_stream_sql_dag.c)
add_edge_test(test_wasm_host_bridge test_wasm_host_bridge.c)
add_edge_test(test_edge_cep_engine test_edge_cep_engine.c)
add_edge_test(test_edge_spiffe_auth test_edge_spiffe_auth.c)
add_edge_test(test_timeseries_compressor test_timeseries_compressor.c)
add_edge_test(test_onnx_host_guard test_onnx_host_guard.c)

