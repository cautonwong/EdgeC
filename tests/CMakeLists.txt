cmake_minimum_required(VERSION 3.10)
project(EdgeC_Tests C)

# 统一测试构建宏
macro(add_edge_test TEST_NAME)
    add_executable(${TEST_NAME} ${ARGN})
    target_link_libraries(${TEST_NAME} PRIVATE
        -Wl,--start-group
        edge_infra
        edge_io
        edge_stream
        edge_security
        edge_core
        edge_mgmt
        edge_bootstrap
        edge_protocol
        edge_algo
        contract_iems
        -Wl,--end-group
        edge_common_interface
        cmocka::cmocka
        pthread dl rt m
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    set_tests_properties(${TEST_NAME} PROPERTIES TIMEOUT 180)
    if(ENABLE_ASAN)
        set_tests_properties(${TEST_NAME} PROPERTIES ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0:halt_on_error=1")
    endif()
    if(ENABLE_TSAN)
        set_tests_properties(${TEST_NAME} PROPERTIES ENVIRONMENT "TSAN_OPTIONS=halt_on_error=0:suppress_equal_stacks=1:die_after_fork=0:exitcode=0")
    endif()
    if(ENABLE_UBSAN)
        set_tests_properties(${TEST_NAME} PROPERTIES ENVIRONMENT "UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1")
    endif()
endmacro()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/CTestCustom.cmake.in ${CMAKE_BINARY_DIR}/CTestCustom.cmake)
set(MEMORYCHECK_SUPPRESSIONS_FILE "${CMAKE_SOURCE_DIR}/valgrind.supp" CACHE FILEPATH "Valgrind suppressions file")

# 1. 核心框架测试 (Framework)
add_edge_test(test_version test_version.c)
add_edge_test(test_is_enabled test_is_enabled.c)
add_edge_test(test_edge_log test_edge_log.c)
add_edge_test(test_net_buf test_net_buf.c)
add_edge_test(test_shmring test_shmring.c)
add_edge_test(test_edge_tlv test_edge_tlv.c)
add_edge_test(test_arbiter test_arbiter.c)
add_edge_test(test_edge_periodic_clock test_edge_periodic_clock.c)
add_edge_test(test_dlt698_axdr test_dlt698_axdr.c)
add_edge_test(test_dcu_inventory test_dcu_inventory.c)
add_edge_test(test_dcu_polling_loop test_dcu_polling_loop.c)
add_edge_test(test_dcu_storage test_dcu_storage.c)
add_edge_test(test_dcu_integration test_dcu_integration.c)
add_edge_test(test_dcu_router test_dcu_router.c)
add_edge_test(test_dcu_advanced_modules test_dcu_advanced_modules.c)
add_edge_test(test_dcu_business_engines test_dcu_business_engines.c)
target_include_directories(test_dcu_business_engines PRIVATE "${CMAKE_SOURCE_DIR}/cmd/dcu")
target_link_libraries(test_dcu_business_engines PRIVATE dcu_app_lib edge_security)

add_edge_test(test_dcu_phase1_delivery test_dcu_phase1_delivery.c)
target_include_directories(test_dcu_phase1_delivery PRIVATE "${CMAKE_SOURCE_DIR}/cmd/dcu" "${CMAKE_SOURCE_DIR}/modules/edge_security/include")
target_link_libraries(test_dcu_phase1_delivery PRIVATE dcu_app_lib edge_security)

add_edge_test(test_dcu_node test_dcu_node.c)
target_include_directories(test_dcu_node PRIVATE "${CMAKE_SOURCE_DIR}/cmd/dcu" "${CMAKE_SOURCE_DIR}/modules/edge_bootstrap/include")
target_link_libraries(test_dcu_node PRIVATE dcu_app_lib edge_bootstrap)

add_edge_test(test_dlt698_conformance_suite test_dlt698_conformance_suite.c)
target_include_directories(test_dlt698_conformance_suite PRIVATE "${CMAKE_SOURCE_DIR}/cmd/dcu")
target_link_libraries(test_dlt698_conformance_suite PRIVATE dcu_app_lib edge_security)





# 2. 数据总线测试 (Data Plane)
add_edge_test(test_datahub test_datahub.c)
add_edge_test(test_datahub_shadow test_datahub_shadow.c)
add_edge_test(test_datahub_multi_backend test_datahub_multi_backend.c)
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
add_edge_test(test_wasm_tensor_abi test_wasm_tensor_abi.c)
add_edge_test(test_sqlite_injection test_sqlite_injection.c)

# 5. 标准算子与算法测试 (Stages & Algos)
add_edge_test(test_linalg test_linalg.c)
add_edge_test(test_algorithm_sum test_algorithm_sum.c)
add_edge_test(test_battery_algo test_battery_algo.c)
add_edge_test(test_iems_business_engines test_iems_business_engines.c)
target_include_directories(test_iems_business_engines PRIVATE "${CMAKE_SOURCE_DIR}/cmd/iems")
target_link_libraries(test_iems_business_engines PRIVATE iems_app_lib)

add_edge_test(test_iems_advanced_delivery test_iems_advanced_delivery.c)
target_include_directories(test_iems_advanced_delivery PRIVATE "${CMAKE_SOURCE_DIR}/cmd/iems")
target_link_libraries(test_iems_advanced_delivery PRIVATE iems_app_lib)

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
    contract_iems
    -Wl,--end-group
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
    contract_iems
    -Wl,--end-group
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
add_edge_test(test_driver_hotplug test_driver_hotplug.c)
add_edge_test(test_impedance_control test_impedance_control.c)
add_edge_test(test_gait_safety test_gait_safety.c)
add_edge_test(test_facility_coordinator test_facility_coordinator.c)
add_edge_test(test_simd_cdd_codec test_simd_cdd_codec.c)
add_edge_test(test_multibus_bridge test_multibus_bridge.c)
add_edge_test(test_stream_sql_dag test_stream_sql_dag.c)
add_edge_test(test_wasm_host_bridge test_wasm_host_bridge.c)
add_edge_test(test_edge_cep_engine test_edge_cep_engine.c)
add_edge_test(test_timeseries_compressor test_timeseries_compressor.c)
add_edge_test(test_onnx_host_guard test_onnx_host_guard.c)
# Automated OMG IDL 4.2 compilation target for clean builds
set(IDLC_BIN "${CMAKE_SOURCE_DIR}/.local/linux-x86_64/bin/idlc")
set(IDL_GEN_DIR "${CMAKE_BINARY_DIR}/idl_generated")
file(MAKE_DIRECTORY "${IDL_GEN_DIR}")

set(IDL_FILES
    "${CMAKE_SOURCE_DIR}/contracts/idl/dlt698_objects.idl"
    "${CMAKE_SOURCE_DIR}/contracts/idl/dlms_bluebook_objects.idl"
    "${CMAKE_SOURCE_DIR}/contracts/idl/idis_pack3_objects.idl"
    "${CMAKE_SOURCE_DIR}/contracts/idl/dlms_cosem_objects.idl"
)

set(IDL_GEN_OUTPUTS
    "${IDL_GEN_DIR}/dlt698_objects.h"
    "${IDL_GEN_DIR}/dlt698_objects.c"
    "${IDL_GEN_DIR}/dlms_bluebook_objects.h"
    "${IDL_GEN_DIR}/dlms_bluebook_objects.c"
    "${IDL_GEN_DIR}/idis_pack3_objects.h"
    "${IDL_GEN_DIR}/idis_pack3_objects.c"
    "${IDL_GEN_DIR}/dlms_cosem_objects.h"
    "${IDL_GEN_DIR}/dlms_cosem_objects.c"
)

if(EXISTS "${IDLC_BIN}")
    add_custom_command(
        OUTPUT ${IDL_GEN_OUTPUTS}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${IDL_GEN_DIR}"
        COMMAND "${IDLC_BIN}" -o "${IDL_GEN_DIR}" "${CMAKE_SOURCE_DIR}/contracts/idl/dlt698_objects.idl"
        COMMAND "${IDLC_BIN}" -o "${IDL_GEN_DIR}" "${CMAKE_SOURCE_DIR}/contracts/idl/dlms_bluebook_objects.idl"
        COMMAND "${IDLC_BIN}" -o "${IDL_GEN_DIR}" "${CMAKE_SOURCE_DIR}/contracts/idl/idis_pack3_objects.idl"
        COMMAND "${IDLC_BIN}" -o "${IDL_GEN_DIR}" "${CMAKE_SOURCE_DIR}/contracts/idl/dlms_cosem_objects.idl"
        DEPENDS ${IDL_FILES}
        COMMENT "Compiling OMG IDL 4.2 files using Cyclone DDS idlc compiler..."
    )
    add_custom_target(generate_idls DEPENDS ${IDL_GEN_OUTPUTS})
    add_edge_test(test_idl_generated_serialization test_idl_generated_serialization.c)
    target_include_directories(test_idl_generated_serialization PRIVATE "${IDL_GEN_DIR}")
    add_dependencies(test_idl_generated_serialization generate_idls)
endif()

# AEP-201 基础设施接口抽象测试
add_edge_test(test_infra_interfaces test_infra_interfaces.c)
target_link_libraries(test_infra_interfaces PRIVATE edge_infra)

# AEP-204 任务装载与环境安全回退定向单测
add_edge_test(test_iems_task_loader test_iems_task_loader.c)
target_include_directories(test_iems_task_loader PRIVATE ${CMAKE_SOURCE_DIR}/cmd/iems ${CMAKE_SOURCE_DIR}/modules/edge_stream/include)
target_link_libraries(test_iems_task_loader PRIVATE iems_app_lib edge_stream edge_security edge_infra SQLite::SQLite3)

# AEP-301 / AEP-302 契约生成式驱动注册表测试 (IEMS, DCU, SmartBox 参数化覆盖)
add_edge_test(test_driver_registry test_driver_registry.c)

add_executable(test_driver_registry_dcu test_driver_registry.c)
target_link_libraries(test_driver_registry_dcu PRIVATE
    -Wl,--start-group
    edge_infra
    edge_io
    edge_stream
    edge_security
    edge_core
    edge_protocol
    edge_algo
    contract_dcu
    -Wl,--end-group
    edge_common_interface
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_driver_registry_dcu COMMAND test_driver_registry_dcu)

add_executable(test_driver_registry_smart_box test_driver_registry.c)
target_link_libraries(test_driver_registry_smart_box PRIVATE
    -Wl,--start-group
    edge_infra
    edge_io
    edge_stream
    edge_security
    edge_core
    edge_protocol
    edge_algo
    contract_smart_box
    -Wl,--end-group
    edge_common_interface
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_driver_registry_smart_box COMMAND test_driver_registry_smart_box)

# AEP-305 生成物可信链与防篡改门禁单测
add_test(NAME test_generated_drift_tamper COMMAND "${CMAKE_SOURCE_DIR}/tests/test_generated_drift_tamper.sh")

# AEP-303 edge-cli 脚手架与契约 linter 集成单测
add_test(NAME test_edge_cli_scaffold COMMAND "${CMAKE_SOURCE_DIR}/tests/test_edge_cli_scaffold.sh")

# AEP-304 DX 自动化集成测试（二开驱动声明即调度）
add_executable(test_dx_scaffolding test_dx_scaffolding.c)
target_link_libraries(test_dx_scaffolding PRIVATE
    -Wl,--start-group
    edge_infra
    edge_io
    edge_stream
    edge_security
    edge_core
    edge_protocol
    edge_algo
    contract_iems
    -Wl,--end-group
    edge_common_interface
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_dx_scaffolding COMMAND test_dx_scaffolding)

# AEP-352 TagID 兼容分域与别名映射单测
add_executable(test_tag_domains_and_aliases test_tag_domains_and_aliases.c)
target_link_libraries(test_tag_domains_and_aliases PRIVATE
    -Wl,--start-group
    edge_infra
    edge_io
    edge_stream
    edge_security
    edge_core
    edge_protocol
    edge_algo
    contract_iems
    -Wl,--end-group
    edge_common_interface
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_tag_domains_and_aliases COMMAND test_tag_domains_and_aliases)

# AEP-353 ACL 防腐层单测 (Anti-Corruption Layer)
add_executable(test_anti_corruption_engine test_anti_corruption_engine.c)
target_link_libraries(test_anti_corruption_engine PRIVATE
    -Wl,--start-group
    edge_infra
    edge_io
    edge_stream
    edge_security
    edge_core
    edge_protocol
    edge_algo
    contract_iems
    -Wl,--end-group
    edge_common_interface
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_anti_corruption_engine COMMAND test_anti_corruption_engine)

# AEP-354 核心业务场景集成测试 (DCU LineLoss/Outage 3011, IEMS Anti-Reverse/TOU Arbitrage)
add_executable(test_domain_scenarios test_domain_scenarios.c)
target_link_libraries(test_domain_scenarios PRIVATE
    -Wl,--start-group
    edge_infra
    edge_io
    edge_stream
    edge_security
    edge_core
    edge_protocol
    edge_algo
    contract_iems
    -Wl,--end-group
    edge_common_interface
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_domain_scenarios COMMAND test_domain_scenarios)

# AEP-355 跨产品 Tag 冲突查重与分域合规单测
add_executable(test_tag_collision_checker test_tag_collision_checker.c)
target_link_libraries(test_tag_collision_checker PRIVATE
    -Wl,--start-group
    edge_infra
    edge_io
    edge_stream
    edge_security
    edge_core
    edge_protocol
    edge_algo
    contract_iems
    -Wl,--end-group
    edge_common_interface
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_tag_collision_checker COMMAND test_tag_collision_checker)

# AEP-401 SDK 公共头文件体系单测 (include/edgeos/edgeos.h)
add_executable(test_sdk_public_headers test_sdk_public_headers.c)
target_link_libraries(test_sdk_public_headers PRIVATE
    -Wl,--start-group
    edge_infra
    edge_io
    edge_stream
    edge_security
    edge_core
    edge_protocol
    edge_algo
    contract_iems
    -Wl,--end-group
    edge_common_interface
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_sdk_public_headers COMMAND test_sdk_public_headers)

# AEP-402 产品应用骨架生成器 (archetype) 单测
add_executable(test_archetype_generator test_archetype_generator.c)
target_link_libraries(test_archetype_generator PRIVATE
    -Wl,--start-group
    edge_infra
    edge_io
    edge_stream
    edge_security
    edge_core
    edge_protocol
    edge_algo
    contract_iems
    -Wl,--end-group
    edge_common_interface
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_archetype_generator COMMAND test_archetype_generator)

# AEP-403 二开驱动与算法范例集成单测
add_executable(test_examples_integration test_examples_integration.c)
target_link_libraries(test_examples_integration PRIVATE
    -Wl,--start-group
    custom_inverter_driver
    custom_soc_filter
    edge_infra
    edge_io
    edge_stream
    edge_security
    edge_core
    edge_protocol
    edge_algo
    contract_iems
    -Wl,--end-group
    edge_common_interface
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_examples_integration COMMAND test_examples_integration)

# AEP-404 契约编译器 DX 诊断与纠错单测
add_executable(test_compiler_dx_diagnostics test_compiler_dx_diagnostics.c)
target_link_libraries(test_compiler_dx_diagnostics PRIVATE
    -Wl,--start-group
    edge_infra
    edge_io
    edge_stream
    edge_security
    edge_core
    edge_protocol
    edge_algo
    contract_iems
    -Wl,--end-group
    edge_common_interface
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_compiler_dx_diagnostics COMMAND test_compiler_dx_diagnostics)

# AEP-405 edge-cli migrate 跨版本迁移向导集成测试
add_executable(test_cli_migrate test_cli_migrate.c)
target_link_libraries(test_cli_migrate PRIVATE
    -Wl,--start-group
    edge_infra
    edge_io
    edge_stream
    edge_security
    edge_core
    edge_algo
    contract_iems
    -Wl,--end-group
    edge_common_interface
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_cli_migrate COMMAND test_cli_migrate)

# AEP-406 二开工程师手册物理一致性自动化测试
add_executable(test_developer_manuals test_developer_manuals.c)
target_link_libraries(test_developer_manuals PRIVATE
    -Wl,--start-group
    edge_infra
    edge_io
    edge_stream
    edge_security
    edge_core
    edge_algo
    contract_iems
    -Wl,--end-group
    edge_common_interface
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_developer_manuals COMMAND test_developer_manuals)

# ARCH-DEEP-01 DataHub 细粒度事件订阅与过滤分发引擎测试与基准
add_executable(test_datahub_subscription test_datahub_subscription.c)
target_link_libraries(test_datahub_subscription PRIVATE
    -Wl,--start-group
    edge_infra
    edge_io
    edge_stream
    edge_security
    edge_core
    edge_algo
    contract_iems
    -Wl,--end-group
    edge_common_interface
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_datahub_subscription COMMAND test_datahub_subscription)

# ARCH-DEEP-02 南向驱动自动轮询调度与遥测编排器测试
add_executable(test_driver_orchestrator test_driver_orchestrator.c)
target_link_libraries(test_driver_orchestrator PRIVATE
    -Wl,--start-group
    edge_infra
    edge_io
    edge_stream
    edge_security
    edge_core
    edge_algo
    contract_iems
    -Wl,--end-group
    edge_common_interface
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_driver_orchestrator COMMAND test_driver_orchestrator)

# ARCH-DEEP-03 时序存储、WAL 崩溃一致性与 SnF 统一仓储测试
add_executable(test_telemetry_store_crash test_telemetry_store_crash.c)
target_link_libraries(test_telemetry_store_crash PRIVATE
    -Wl,--start-group
    edge_infra
    edge_io
    edge_stream
    edge_security
    edge_core
    edge_algo
    contract_iems
    -Wl,--end-group
    edge_common_interface
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_telemetry_store_crash COMMAND test_telemetry_store_crash)

# ARCH-DEEP-04 流计算 DAG 管道声明式自装配引擎测试
add_executable(test_stream_yaml_assembly test_stream_yaml_assembly.c)
target_link_libraries(test_stream_yaml_assembly PRIVATE
    -Wl,--start-group
    edge_infra
    edge_io
    edge_stream
    edge_security
    edge_core
    edge_algo
    contract_iems
    -Wl,--end-group
    edge_common_interface
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_stream_yaml_assembly COMMAND test_stream_yaml_assembly)

# ARCH-DDS-01 Cyclone DDS 工业级能力深化与分布式网状通信测试
if(CONFIG_EDGE_INFRA_DDS AND HAVE_DDS_HEADERS)
add_executable(test_datahub_dds_mesh test_datahub_dds_mesh.c)
target_include_directories(test_datahub_dds_mesh PRIVATE
    ${CMAKE_BINARY_DIR}/idl_generated
)
target_link_libraries(test_datahub_dds_mesh PRIVATE
    -Wl,--start-group
    edge_infra
    edge_io
    edge_stream
    edge_security
    edge_core
    edge_algo
    contract_iems
    -Wl,--end-group
    edge_common_interface
    cyclonedds
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_datahub_dds_mesh COMMAND test_datahub_dds_mesh)

# AEP-450: Eclipse Cyclone DDS IDL 强类型契约编译闭环与 C CDR 描述符生成测试 (Issue #142)
add_executable(test_dds_idl_codec test_dds_idl_codec.c)
target_include_directories(test_dds_idl_codec PRIVATE
    ${CMAKE_SOURCE_DIR}/contracts/idl/generated
    ${CMAKE_BINARY_DIR}/idl_generated
)
target_link_libraries(test_dds_idl_codec PRIVATE
    cyclonedds
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_dds_idl_codec COMMAND test_dds_idl_codec)

# AEP-451: 工业四遥分级 QoS 策略矩阵与自适应 DataWriter/DataReader 引擎测试 (Issue #143)
add_executable(test_dds_qos_matrix test_dds_qos_matrix.c)
target_include_directories(test_dds_qos_matrix PRIVATE
    ${CMAKE_SOURCE_DIR}/contracts/idl/generated
    ${CMAKE_BINARY_DIR}/idl_generated
    ${CMAKE_SOURCE_DIR}/modules/edge_infra/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)
target_link_libraries(test_dds_qos_matrix PRIVATE
    -Wl,--start-group
    edge_infra
    edge_io
    edge_stream
    edge_security
    edge_core
    edge_algo
    contract_iems
    -Wl,--end-group
    edge_common_interface
    cyclonedds
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_dds_qos_matrix COMMAND test_dds_qos_matrix)

# AEP-453: DDS 零拷贝共享内存 (Iceoryx/SHM) 加速与跨网络透明双轨路由测试 (Issue #145)
add_executable(test_dds_shm_zerocopy_perf test_dds_shm_zerocopy_perf.c)
target_include_directories(test_dds_shm_zerocopy_perf PRIVATE
    ${CMAKE_SOURCE_DIR}/contracts/idl/generated
    ${CMAKE_BINARY_DIR}/idl_generated
    ${CMAKE_SOURCE_DIR}/modules/edge_infra/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)
target_link_libraries(test_dds_shm_zerocopy_perf PRIVATE
    -Wl,--start-group
    edge_infra
    edge_io
    edge_stream
    edge_security
    edge_core
    edge_algo
    contract_iems
    -Wl,--end-group
    edge_common_interface
    cyclonedds
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_dds_shm_zerocopy_perf COMMAND test_dds_shm_zerocopy_perf)
endif()

# AEP-440 IEMS 工商业储能需量控制、分时电价排程与 100ms 快速防逆流测试
add_executable(test_iems_energy_strategy test_iems_energy_strategy.c ../cmd/iems/iems_microgrid_coordinator.c)
target_include_directories(test_iems_energy_strategy PRIVATE
    ${CMAKE_SOURCE_DIR}/cmd/iems
)
target_link_libraries(test_iems_energy_strategy PRIVATE
    -Wl,--start-group
    edge_infra
    edge_io
    edge_stream
    edge_security
    edge_core
    edge_algo
    contract_iems
    -Wl,--end-group
    edge_common_interface
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_iems_energy_strategy COMMAND test_iems_energy_strategy)

# AEP-441 DCU 配网台区三相不平衡换相决策与 31 次谐波/暂态电能质量分析测试
add_executable(test_dcu_power_quality test_dcu_power_quality.c ../cmd/dcu/dcu_power_quality.c)
target_include_directories(test_dcu_power_quality PRIVATE
    ${CMAKE_SOURCE_DIR}/cmd/dcu
)
target_link_libraries(test_dcu_power_quality PRIVATE
    -Wl,--start-group
    edge_infra
    edge_io
    edge_stream
    edge_security
    edge_core
    edge_algo
    contract_dcu
    -Wl,--end-group
    edge_common_interface
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_dcu_power_quality COMMAND test_dcu_power_quality)

# AEP-442 SmartBox 箱变绕组热寿命老化评估模型与电气五防智能闭锁测试
add_executable(test_smartbox_interlock_thermal test_smartbox_interlock_thermal.c ../cmd/smart_box/smartbox_thermal_aging.c ../cmd/smart_box/smartbox_five_safety_interlock.c)
target_include_directories(test_smartbox_interlock_thermal PRIVATE
    ${CMAKE_SOURCE_DIR}/cmd/smart_box
)
target_link_libraries(test_smartbox_interlock_thermal PRIVATE
    -Wl,--start-group
    edge_infra
    edge_io
    edge_stream
    edge_security
    edge_core
    edge_algo
    contract_smart_box
    -Wl,--end-group
    edge_common_interface
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_smartbox_interlock_thermal COMMAND test_smartbox_interlock_thermal)

# AEP-452 Cyclone DDS 节点动态自发现、Liveliness 监听与分布式拓扑自愈测试
add_executable(test_dds_discovery_liveliness test_dds_discovery_liveliness.c)
target_include_directories(test_dds_discovery_liveliness PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_infra/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)
target_link_libraries(test_dds_discovery_liveliness PRIVATE
    -Wl,--start-group
    edge_infra
    edge_io
    edge_stream
    edge_security
    edge_core
    edge_algo
    -Wl,--end-group
    edge_common_interface
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_dds_discovery_liveliness COMMAND test_dds_discovery_liveliness)

# AEP-454 DDS 现场运维观测与 edge-cli dds 诊断测试
add_executable(test_dds_cli_diagnostics test_dds_cli_diagnostics.c)
target_link_libraries(test_dds_cli_diagnostics PRIVATE
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_dds_cli_diagnostics COMMAND test_dds_cli_diagnostics)

# AEP-490 EdgeOS Web 设计系统 Tokens 与深黑主题测试
add_executable(test_web_design_tokens test_web_design_tokens.c)
target_link_libraries(test_web_design_tokens PRIVATE
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_web_design_tokens COMMAND test_web_design_tokens)

# AEP-491 EdgeOS Web 契约驱动与 Pinia ContractLoader 动态聚合测试
add_executable(test_web_contract_loader test_web_contract_loader.c)
target_link_libraries(test_web_contract_loader PRIVATE
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_web_contract_loader COMMAND test_web_contract_loader)

# AEP-492 LiveTagCard 工业级高刷组件与 400ms GSAP 动效及 shallowRef 内存优化测试
add_executable(test_web_live_tag_card test_web_live_tag_card.c)
target_link_libraries(test_web_live_tag_card PRIVATE
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_web_live_tag_card COMMAND test_web_live_tag_card)

# AEP-493 DataCurves ECharts 工业级负荷曲线与 LTTB 动态降采样及十字准星测试
add_executable(test_web_data_curves test_web_data_curves.c)
target_link_libraries(test_web_data_curves PRIVATE
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_web_data_curves COMMAND test_web_data_curves)

# AEP-494 IEMS Sankey 动态能流图与 TOU 分时套利看板测试
add_executable(test_web_iems_energy_flow test_web_iems_energy_flow.c)
target_link_libraries(test_web_iems_energy_flow PRIVATE
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_web_iems_energy_flow COMMAND test_web_iems_energy_flow)

# AEP-495 SmartBox 100Hz 示波器与双人复核安全沙盒测试
add_executable(test_smartbox_scope_double_auth test_smartbox_scope_double_auth.c)
target_link_libraries(test_smartbox_scope_double_auth PRIVATE
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_smartbox_scope_double_auth COMMAND test_smartbox_scope_double_auth)

# AEP-496 DCU DL/T 645/698 规约抓包器与三相谐波电能质量/换相看板测试
add_executable(test_web_dcu_power_quality test_web_dcu_power_quality.c ../cmd/dcu/dcu_power_quality.c)
target_include_directories(test_web_dcu_power_quality PRIVATE
    ${CMAKE_SOURCE_DIR}/cmd/dcu
)
target_link_libraries(test_web_dcu_power_quality PRIVATE
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_web_dcu_power_quality COMMAND test_web_dcu_power_quality)

# AEP-497 全局实时 SOE 毫秒级事件流中心与告警瀑布流看板测试
add_executable(test_web_event_stream test_web_event_stream.c ../cmd/dcu/dcu_soe_event_engine.c)
target_include_directories(test_web_event_stream PRIVATE
    ${CMAKE_SOURCE_DIR}/cmd/dcu
)
target_link_libraries(test_web_event_stream PRIVATE
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_web_event_stream COMMAND test_web_event_stream)

# AEP-498 Web ANSI 实时日志流控制台与过滤导出终端测试
add_executable(test_web_log_terminal test_web_log_terminal.c)
target_include_directories(test_web_log_terminal PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)
target_link_libraries(test_web_log_terminal PRIVATE
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_web_log_terminal COMMAND test_web_log_terminal)

# AEP-499 全景实时系统级 Metrics 矩阵与 DataHub/DDS 资源监控看板测试
add_executable(test_web_metrics_matrix test_web_metrics_matrix.c)
target_include_directories(test_web_metrics_matrix PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)
target_link_libraries(test_web_metrics_matrix PRIVATE
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_web_metrics_matrix COMMAND test_web_metrics_matrix)

# AEP-500 南向物理通道通信拓扑与 HPLC/无线信噪比热力图交互看板测试
add_executable(test_web_channel_topology test_web_channel_topology.c)
target_include_directories(test_web_channel_topology PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)
target_link_libraries(test_web_channel_topology PRIVATE
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_web_channel_topology COMMAND test_web_channel_topology)

# AEP-501 全局抽屉式 Web 交互控制台与快捷按键召唤底座测试
add_executable(test_web_console_drawer test_web_console_drawer.c)
target_include_directories(test_web_console_drawer PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)
target_link_libraries(test_web_console_drawer PRIVATE
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_web_console_drawer COMMAND test_web_console_drawer)

# AEP-502 交互式 CLI Shell 引擎与 WebSocket 双向协议通道测试
add_executable(test_web_cli_bridge test_web_cli_bridge.c)
target_include_directories(test_web_cli_bridge PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_mgmt/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)
target_link_libraries(test_web_cli_bridge PRIVATE
    edge_mgmt
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_web_cli_bridge COMMAND test_web_cli_bridge)

# AEP-503 工业协议原始报文抓包与实时十六进制解析器测试
add_executable(test_web_protocol_sniffer test_web_protocol_sniffer.c)
target_include_directories(test_web_protocol_sniffer PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_mgmt/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)
target_link_libraries(test_web_protocol_sniffer PRIVATE
    edge_mgmt
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_web_protocol_sniffer COMMAND test_web_protocol_sniffer)

# AEP-504 工业控制台安全门禁、高危操作双人复核与全量审计留痕测试
add_executable(test_web_console_rbac_audit test_web_console_rbac_audit.c)
target_include_directories(test_web_console_rbac_audit PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_mgmt/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)
target_link_libraries(test_web_console_rbac_audit PRIVATE
    edge_mgmt
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_web_console_rbac_audit COMMAND test_web_console_rbac_audit)

# AEP-506 工业 MCP Server 分发器与双轨传输层测试
add_executable(test_mcp_dispatcher test_mcp_dispatcher.c)
target_include_directories(test_mcp_dispatcher PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_mcp/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
    ${CMAKE_SOURCE_DIR}/modules/edge_io/include
    ${CMAKE_SOURCE_DIR}/modules/edge_stream/include
    ${CMAKE_SOURCE_DIR}/modules/edge_infra/include
)
target_link_libraries(test_mcp_dispatcher PRIVATE
    edge_mcp
    edge_core
    edge_io
    edge_stream
    edge_infra
    cjson
    libuv::uv_a
    pthread dl rt m
)
add_test(NAME test_mcp_dispatcher COMMAND test_mcp_dispatcher)

add_executable(test_mcp_server_protocol test_mcp_server_protocol.c)
target_include_directories(test_mcp_server_protocol PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_mcp/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
    ${CMAKE_SOURCE_DIR}/modules/edge_io/include
    ${CMAKE_SOURCE_DIR}/modules/edge_stream/include
    ${CMAKE_SOURCE_DIR}/modules/edge_infra/include
)
target_link_libraries(test_mcp_server_protocol PRIVATE
    edge_mcp
    edge_core
    edge_io
    edge_stream
    edge_infra
    cjson
    libuv::uv_a
    pthread dl rt m
)
add_test(NAME test_mcp_server_protocol COMMAND test_mcp_server_protocol)

# AEP-510 工业共享 MCP 总线与多客户端 UDS 传输层单测
add_executable(test_mcp_uds test_mcp_uds.c)
target_include_directories(test_mcp_uds PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_mcp/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
    ${CMAKE_SOURCE_DIR}/modules/edge_io/include
    ${CMAKE_SOURCE_DIR}/modules/edge_stream/include
    ${CMAKE_SOURCE_DIR}/modules/edge_infra/include
)
target_link_libraries(test_mcp_uds PRIVATE
    edge_mcp
    edge_core
    edge_io
    edge_stream
    edge_infra
    cjson
    libuv::uv_a
    pthread dl rt m
)
add_test(NAME test_mcp_uds COMMAND test_mcp_uds)

# AEP-507 工业 AI 伴侣抽屉与 MCP 工具调用组件测试
add_executable(test_web_ai_copilot test_web_ai_copilot.c)
target_link_libraries(test_web_ai_copilot PRIVATE
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_web_ai_copilot COMMAND test_web_ai_copilot)

# AEP-508 行业专有算法 MCP 工具链单测
add_executable(test_mcp_industry_tools test_mcp_industry_tools.c)
target_include_directories(test_mcp_industry_tools PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_mcp/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
    ${CMAKE_SOURCE_DIR}/modules/edge_io/include
    ${CMAKE_SOURCE_DIR}/modules/edge_stream/include
    ${CMAKE_SOURCE_DIR}/modules/edge_infra/include
)
target_link_libraries(test_mcp_industry_tools PRIVATE
    edge_mcp
    edge_core
    edge_io
    edge_stream
    edge_infra
    cjson
    libuv::uv_a
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_mcp_industry_tools COMMAND test_mcp_industry_tools)

# AEP-731 分数阶等效电路 FO-ECM 在线参数自适应卡尔曼滤波单测
add_executable(test_battery_fo_ecm_ekf test_battery_fo_ecm_ekf.c)
target_include_directories(test_battery_fo_ecm_ekf PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_algo/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)
target_link_libraries(test_battery_fo_ecm_ekf PRIVATE
    edge_algo
    edge_core
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_battery_fo_ecm_ekf COMMAND test_battery_fo_ecm_ekf)

# AEP-732 储能电芯析锂微内短路 1~7 天早期预警模型与残差辨识核单测
add_executable(test_battery_micro_isc test_battery_micro_isc.c)
target_include_directories(test_battery_micro_isc PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_algo/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)
target_link_libraries(test_battery_micro_isc PRIVATE
    edge_algo
    edge_core
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_battery_micro_isc COMMAND test_battery_micro_isc)

# AEP-733 4096 节点电芯空间矩阵与热失控扩散预测单测
add_executable(test_battery_spatial_matrix test_battery_spatial_matrix.c)
target_include_directories(test_battery_spatial_matrix PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_algo/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)
target_link_libraries(test_battery_spatial_matrix PRIVATE
    edge_algo
    edge_core
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_battery_spatial_matrix COMMAND test_battery_spatial_matrix)

# AEP-734 在线电化学阻抗谱 (EIS) 宽频 PRBS 快速辨识单测
add_executable(test_battery_eis test_battery_eis.c)
target_include_directories(test_battery_eis PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_algo/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)
target_link_libraries(test_battery_eis PRIVATE
    edge_algo
    edge_core
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_battery_eis COMMAND test_battery_eis)

# AEP-735 动力/储能电池健康状态 (SOH) 与剩余寿命 (RUL) 融合估计单测
add_executable(test_battery_soh_fusion test_battery_soh_fusion.c)
target_include_directories(test_battery_soh_fusion PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_algo/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)
target_link_libraries(test_battery_soh_fusion PRIVATE
    edge_algo
    edge_core
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_battery_soh_fusion COMMAND test_battery_soh_fusion)

# AEP-736 多电芯被动/主动均衡最优能效与寿命均衡控制单测
add_executable(test_battery_balancer test_battery_balancer.c)
target_include_directories(test_battery_balancer PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_algo/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)
target_link_libraries(test_battery_balancer PRIVATE
    edge_algo
    edge_core
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_battery_balancer COMMAND test_battery_balancer)

# AEP-737 电池热失控喷阀前气体多源融合概率预警单测
add_executable(test_battery_gas_early_warning test_battery_gas_early_warning.c)
target_include_directories(test_battery_gas_early_warning PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_algo/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)
target_link_libraries(test_battery_gas_early_warning PRIVATE
    edge_algo
    edge_core
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_battery_gas_early_warning COMMAND test_battery_gas_early_warning)

# AEP-738 极端低温环境电池脉冲自加热热电耦合控制单测
add_executable(test_battery_pulse_heating test_battery_pulse_heating.c)
target_include_directories(test_battery_pulse_heating PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_algo/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)
target_link_libraries(test_battery_pulse_heating PRIVATE
    edge_algo
    edge_core
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_battery_pulse_heating COMMAND test_battery_pulse_heating)

# AEP-739 电池单体热阻抗矩阵非均匀热场三维重建单测
add_executable(test_battery_thermal_mesh test_battery_thermal_mesh.c)
target_include_directories(test_battery_thermal_mesh PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_algo/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)
target_link_libraries(test_battery_thermal_mesh PRIVATE
    edge_algo
    edge_core
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_battery_thermal_mesh COMMAND test_battery_thermal_mesh)

# AEP-740 电池峰值充放电功率 SOP 多约束物理边界求解器单测
add_executable(test_battery_sop_dynamic_bounds test_battery_sop_dynamic_bounds.c)
target_include_directories(test_battery_sop_dynamic_bounds PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_algo/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)
target_link_libraries(test_battery_sop_dynamic_bounds PRIVATE
    edge_algo
    edge_core
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_battery_sop_dynamic_bounds COMMAND test_battery_sop_dynamic_bounds)

# ARCH-MCP-02 工业级共享 MCP 神经总线与安全门禁单测
add_executable(test_mcp_bus_security test_mcp_bus_security.c)
target_include_directories(test_mcp_bus_security PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_mcp/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
    ${CMAKE_SOURCE_DIR}/modules/edge_io/include
    ${CMAKE_SOURCE_DIR}/modules/edge_stream/include
    ${CMAKE_SOURCE_DIR}/modules/edge_infra/include
)
target_link_libraries(test_mcp_bus_security PRIVATE
    edge_mcp
    edge_core
    edge_io
    edge_stream
    edge_infra
    cjson
    libuv::uv_a
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_mcp_bus_security COMMAND test_mcp_bus_security)

# ARCH-MCP-03 微服务 Provider 动态挂载与工业原子工具集成单测
add_executable(test_mcp_microservices_integration test_mcp_microservices_integration.c)
target_include_directories(test_mcp_microservices_integration PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_mcp/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
    ${CMAKE_SOURCE_DIR}/modules/edge_io/include
    ${CMAKE_SOURCE_DIR}/modules/edge_stream/include
    ${CMAKE_SOURCE_DIR}/modules/edge_infra/include
)
target_link_libraries(test_mcp_microservices_integration PRIVATE
    edge_mcp
    edge_core
    edge_io
    edge_stream
    edge_infra
    cjson
    libuv::uv_a
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_mcp_microservices_integration COMMAND test_mcp_microservices_integration)

# AEP-741 华为昇腾 (Ascend ACL) NPU 统一驱动契约与零拷贝张量映射测试
add_executable(test_npu_ascend_acl test_npu_ascend_acl.c)
target_include_directories(test_npu_ascend_acl PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_infra/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)
target_link_libraries(test_npu_ascend_acl PRIVATE
    edge_infra
    edge_core
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_npu_ascend_acl COMMAND test_npu_ascend_acl)

# AEP-742 瑞芯微 (Rockchip RKNN) NPU 硬件多核负载均衡与 DMA-BUF 绑定测试 (Issue #272)
add_executable(test_npu_rockchip_rknn test_npu_rockchip_rknn.c)
target_include_directories(test_npu_rockchip_rknn PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_infra/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)
target_link_libraries(test_npu_rockchip_rknn PRIVATE
    edge_infra
    edge_core
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_npu_rockchip_rknn COMMAND test_npu_rockchip_rknn)

# AEP-743 地平线 (Horizon BPU) 嵌入式异构计算接入测试 (Issue #273)
add_executable(test_npu_horizon_bpu test_npu_horizon_bpu.c)
target_include_directories(test_npu_horizon_bpu PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_infra/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)
target_link_libraries(test_npu_horizon_bpu PRIVATE
    edge_infra
    edge_core
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_npu_horizon_bpu COMMAND test_npu_horizon_bpu)

# AEP-744 英伟达 (NVIDIA Jetson) TensorRT 异步推理与 CUDA 流绑定测试 (Issue #274)
add_executable(test_npu_nvidia_trt test_npu_nvidia_trt.c)
target_include_directories(test_npu_nvidia_trt PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_infra/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)
target_link_libraries(test_npu_nvidia_trt PRIVATE
    edge_infra
    edge_core
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_npu_nvidia_trt COMMAND test_npu_nvidia_trt)

# AEP-745 德州仪器 (TI Sitara) C7x DSP / MMA 矩阵加速器测试 (Issue #275)
add_executable(test_npu_ti_sitara test_npu_ti_sitara.c)
target_include_directories(test_npu_ti_sitara PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_infra/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)
target_link_libraries(test_npu_ti_sitara PRIVATE
    edge_infra
    edge_core
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_npu_ti_sitara COMMAND test_npu_ti_sitara)

# AEP-746 Linux DMA-BUF 内存文件描述符生命周期管理器与内存池测试 (Issue #276)
add_executable(test_dma_buf_pool test_dma_buf_pool.c)
target_include_directories(test_dma_buf_pool PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_infra/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)
target_link_libraries(test_dma_buf_pool PRIVATE
    edge_infra
    edge_core
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_dma_buf_pool COMMAND test_dma_buf_pool)

# AEP-610: NPU HAL & DMA-BUF Zero-Copy Hardware Acceleration (Issue #229)
add_executable(test_npu_hal test_npu_hal.c)
target_include_directories(test_npu_hal PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_infra/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)
target_link_libraries(test_npu_hal PRIVATE
    edge_infra
    edge_core
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_npu_hal COMMAND test_npu_hal)

# AEP-747 NPU 硬件异常过温/死锁自愈机制与 CPU 备用推理核测试 (Issue #277)
add_executable(test_npu_self_healing test_npu_self_healing.c)
target_include_directories(test_npu_self_healing PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_infra/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)
target_link_libraries(test_npu_self_healing PRIVATE
    edge_infra
    edge_core
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_npu_self_healing COMMAND test_npu_self_healing)

# AEP-748: NPU PMU Hardware Performance Counters Test (Issue #278)
add_executable(test_npu_pmu test_npu_pmu.c)
target_include_directories(test_npu_pmu PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_infra/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)
target_link_libraries(test_npu_pmu PRIVATE
    edge_infra
    edge_core
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_npu_pmu COMMAND test_npu_pmu)

# AEP-749: NPU Model Encryption & Secure Enclave Test (Issue #279)
add_executable(test_npu_enclave test_npu_enclave.c)
target_include_directories(test_npu_enclave PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_infra/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)
target_link_libraries(test_npu_enclave PRIVATE
    edge_infra
    edge_core
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_npu_enclave COMMAND test_npu_enclave)

# AEP-750: NPU Power Manager & Dynamic Clock Gating Test (Issue #280)
add_executable(test_npu_power_manager test_npu_power_manager.c)
target_include_directories(test_npu_power_manager PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_infra/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)
target_link_libraries(test_npu_power_manager PRIVATE
    edge_infra
    edge_core
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_npu_power_manager COMMAND test_npu_power_manager)

# AEP-751 组合根 (Composition Root) 与消费者定义接口 (Consumer-Defined Ports) 测试 (Issue #336)
add_executable(test_composition_root test_composition_root.c)
target_include_directories(test_composition_root PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_bootstrap/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
    ${CMAKE_SOURCE_DIR}/modules/edge_algo/include
    ${CMAKE_SOURCE_DIR}/modules/edge_infra/include
)
target_link_libraries(test_composition_root PRIVATE
    edge_bootstrap
    edge_core
    edge_algo
    edge_infra
    cmocka::cmocka
    pthread dl rt m
)
add_test(NAME test_composition_root COMMAND test_composition_root)

# AEP-792 架构质量守卫与反驳测试 (Testing the Gates) (Issue #337)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_test(NAME test_ci_architecture_guards
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/ci/scripts/run_guard_selftest.py
)

# AEP-752 调用方提供内存 (Caller-Allocated Storage) 与不透明指针物理封装架构单测 (Issue #340)
add_executable(test_caller_allocated_storage test_caller_allocated_storage.c)
target_include_directories(test_caller_allocated_storage PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_bootstrap/include
    ${CMAKE_SOURCE_DIR}/modules/edge_algo/include
    ${CMAKE_SOURCE_DIR}/modules/edge_stream/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
    ${CMAKE_SOURCE_DIR}/modules/edge_infra/include
    ${CMAKE_SOURCE_DIR}/modules/edge_io/include
    ${CMAKE_SOURCE_DIR}/modules/edge_security/include
    ${CMAKE_SOURCE_DIR}/modules/edge_protocol/include
)
target_link_libraries(test_caller_allocated_storage PRIVATE
    -Wl,--start-group
    edge_bootstrap
    edge_algo
    edge_stream
    edge_core
    edge_infra
    edge_io
    edge_security
    edge_protocol
    contract_iems
    -Wl,--end-group
    cmocka::cmocka
    libuv::uv_a
    pthread dl rt m
)
add_test(NAME test_caller_allocated_storage COMMAND test_caller_allocated_storage)

# AEP-723 ~ AEP-730 构网型虚拟同步发电机 (VSG) 控制算法单测
add_edge_test(test_vsg_control test_vsg_control.c)

# AEP-720 确定性任务调度死线监控与熔断器单测
add_edge_test(test_edge_sched_deadline test_edge_sched_deadline.c)

# AEP-711 PREEMPT_RT 优先级反转天花板协议 (PCP) 互斥锁与审计单测
add_edge_test(test_pcp_mutex test_pcp_mutex.c)

# AEP-712 最早截止时间优先 (EDF) 动态硬实时调度器单测
add_edge_test(test_edf_scheduler test_edf_scheduler.c)

# AEP-713 单调速率 (RMS) 静态确定性周期调度器与时间预算看门狗单测
add_edge_test(test_rms_scheduler test_rms_scheduler.c)

# AEP-716 CPU 核心独占绑定、中断亲和性 (IRQ) 调优与内存锁定 (mlockall) 单测
add_edge_test(test_cpu_affinity test_cpu_affinity.c)

# AEP-718 突发工况事件优先级动态提升 (Priority Boosting) 与退火衰减单测
add_edge_test(test_prio_booster test_prio_booster.c)

# AEP-719 多任务协作式调度时间片超限软熔断与降级回调单测
add_edge_test(test_coop_guard test_coop_guard.c)

# AEP-602 工业混合关键度实时调度器 (Mixed-Criticality Dispatcher) 与 MTT 确定性保障单测
add_edge_test(test_mixed_criticality test_mixed_criticality.c)

# AEP-702 INT8/FP8 对称与非对称量化校准器单测
add_edge_test(test_ai_quant_calibrator test_ai_quant_calibrator.c)

# AEP-703 边缘端 4-bit (AWQ/GPTQ) 紧凑权重量化与 GEMV 单测
add_edge_test(test_ai_awq_gptq_gemm test_ai_awq_gptq_gemm.c)

# AEP-704 工业时序多尺度 1D-CNN + BiLSTM 故障诊断单测
add_edge_test(test_ai_temporal_fault_net test_ai_temporal_fault_net.c)

# AEP-706 动态张量形状内存池静态预分配器单测
add_edge_test(test_ai_dynamic_tensor_pool test_ai_dynamic_tensor_pool.c)

# AEP-707 边缘端轻量小语言模型 (SLM) KV-Cache 压缩与滑动窗口注意力单测
add_edge_test(test_ai_slm_kv_cache test_ai_slm_kv_cache.c)

# AEP-708 神经网络计算图拓扑剪枝与死算子无死锁剔除单测
add_edge_test(test_ai_graph_pruner test_ai_graph_pruner.c)

# AEP-705 基于 CMSIS-NN 的 ARM Cortex 极速矩阵乘法 (GEMM) 汇编优化单测
add_edge_test(test_ai_fast_gemm test_ai_fast_gemm.c)

# AEP-611 SIL-3 级三模冗余 (TMR) 锁步容错与无扰故障转移测试 (Issue #230)
add_edge_test(test_tmr_failover test_tmr_failover.c)

# AEP-612 基于 eBPF 的内核微秒级调度抖动遥测与自适应实时熔断器测试 (Issue #231)
add_edge_test(test_ebpf_telemetry test_ebpf_telemetry.c)

# AEP-614 工业恶劣工况热感知动态功耗预算与自适应降级调度器测试 (Issue #233)
add_edge_test(test_thermal_scheduler test_thermal_scheduler.c)

# AEP-613 ISA-95 统一命名空间 (UNS) 与 Sparkplug B / OPC UA 镜像单测
add_edge_test(test_uns_sparkplug_opcua test_uns_sparkplug_opcua.c)

# AEP-609 IEEE 802.1Qbv TAS 与 IEEE 802.1AS TSN 确定性传输单测
add_edge_test(test_tsn_engine test_tsn_engine.c)

# AEP-605 异构多核非对称协同 (AMP/SMP) 与核间零拷贝通信单测
add_edge_test(test_amp_ipc_ring test_amp_ipc_ring.c)

# AEP-607 VSG 构网型虚拟同步机与 FFR 快速调频单测
add_edge_test(test_vsg_ffr test_vsg_ffr.c)

# AEP-606 基于 DDS/TSN 的 Swarm AI 协同调度与任务卸载单测
add_edge_test(test_swarm_ai_offloading test_swarm_ai_offloading.c)

# AEP-604 量化算子融合与无锁模型热插拔单测
add_edge_test(test_operator_fusion test_operator_fusion.c)

# AEP-608 储能电芯热失控 PINN 早期预警单测
add_edge_test(test_battery_thermal_pinn test_battery_thermal_pinn.c)

# AEP-601 Wasm-NN 共享内存零拷贝沙箱单测
add_edge_test(test_wasm_nn_sandbox test_wasm_nn_sandbox.c)

# AEP-603 CDD 多模态张量流水线与 DAG 拓扑单测
add_edge_test(test_cdd_tensor_schema test_cdd_tensor_schema.c)



# AEP-709 工业端侧物理引导神经网络 (PINN) 质量守恒惩罚项求导器单测
add_edge_test(test_ai_pinn_evaluator test_ai_pinn_evaluator.c)

# AEP-710 异构多后端 (NPU/GPU/CPU) 算子级自动切分与流水线编排器单测
add_edge_test(test_ai_hetero_pipeline test_ai_hetero_pipeline.c)

# ARCH-GOV: 聚合根封装治理单测 (Issue #369, #370)
add_edge_test(test_aggregate_roots_governance test_aggregate_roots_governance.c)

# 阶段 2：电网物联高韧性与型式试验实战战役测试套件
add_executable(test_dcu_mutex_marathon test_dcu_mutex_marathon.c)
target_include_directories(test_dcu_mutex_marathon PRIVATE "${CMAKE_SOURCE_DIR}/cmd/dcu")
target_link_libraries(test_dcu_mutex_marathon PRIVATE dcu_app_lib edge_core pthread)
add_test(NAME test_dcu_mutex_marathon COMMAND test_dcu_mutex_marathon)

add_executable(test_dcu_topology_phase_id test_dcu_topology_phase_id.c)
target_include_directories(test_dcu_topology_phase_id PRIVATE "${CMAKE_SOURCE_DIR}/cmd/dcu")
target_link_libraries(test_dcu_topology_phase_id PRIVATE dcu_app_lib m)
add_test(NAME test_dcu_topology_phase_id COMMAND test_dcu_topology_phase_id)

add_executable(test_bus_resilience_recovery test_bus_resilience_recovery.c)
target_include_directories(test_bus_resilience_recovery PRIVATE "${CMAKE_SOURCE_DIR}/modules/edge_infra/include")
target_link_libraries(test_bus_resilience_recovery PRIVATE edge_infra)
add_test(NAME test_bus_resilience_recovery COMMAND test_bus_resilience_recovery)

add_executable(test_northbound_dual_track_snf test_northbound_dual_track_snf.c "${CMAKE_SOURCE_DIR}/services/northbound/northbound_dual_track_snf.c")
target_include_directories(test_northbound_dual_track_snf PRIVATE "${CMAKE_SOURCE_DIR}/services/northbound")
add_test(NAME test_northbound_dual_track_snf COMMAND test_northbound_dual_track_snf)

add_executable(test_iec104_type_test_suite test_iec104_type_test_suite.c)
target_include_directories(test_iec104_type_test_suite PRIVATE "${CMAKE_SOURCE_DIR}/modules/edge_protocol/include" "${CMAKE_SOURCE_DIR}/modules/edge_io/include")
target_link_libraries(test_iec104_type_test_suite PRIVATE edge_protocol edge_core)
add_test(NAME test_iec104_type_test_suite COMMAND test_iec104_type_test_suite)

add_executable(test_storage_powercut_resilience test_storage_powercut_resilience.c "${CMAKE_SOURCE_DIR}/tools/fault_injector/storage_powercut_fault_injector.c")
target_include_directories(test_storage_powercut_resilience PRIVATE "${CMAKE_SOURCE_DIR}/tools/fault_injector")
target_link_libraries(test_storage_powercut_resilience PRIVATE SQLite::SQLite3)
add_test(NAME test_storage_powercut_resilience COMMAND test_storage_powercut_resilience)

add_executable(test_soak_memory_convergence test_soak_memory_convergence.c)
target_link_libraries(test_soak_memory_convergence PRIVATE edge_core)
add_test(NAME test_soak_memory_convergence COMMAND test_soak_memory_convergence)

foreach(t
    test_dcu_mutex_marathon
    test_dcu_topology_phase_id
    test_bus_resilience_recovery
    test_northbound_dual_track_snf
    test_iec104_type_test_suite
    test_storage_powercut_resilience
    test_soak_memory_convergence
)
    set_tests_properties(${t} PROPERTIES TIMEOUT 180)
    if(ENABLE_ASAN)
        set_tests_properties(${t} PROPERTIES ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0:halt_on_error=1")
    endif()
    if(ENABLE_TSAN)
        set_tests_properties(${t} PROPERTIES ENVIRONMENT "TSAN_OPTIONS=halt_on_error=0:suppress_equal_stacks=1:die_after_fork=0:exitcode=0")
    endif()
    if(ENABLE_UBSAN)
        set_tests_properties(${t} PROPERTIES ENVIRONMENT "UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1")
    endif()
endforeach()
