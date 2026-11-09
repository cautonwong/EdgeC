# modules/edge_algo/CMakeLists.txt

set(ALGO_SOURCES
    src/algorithm_sum.c
    src/battery_strategy.c
    src/battery_fo_ecm_ekf.c
    src/vsg_control.c
    src/vsg_presync.c
    src/vsg_black_start.c
    src/vsg_circulating.c
    src/vsg_sso.c
    src/vsg_frt.c
    src/vsg_virtual_z.c
    src/vsg_compute_tracking.c
    src/battery_micro_isc.c
    src/battery_spatial_matrix.c
    src/battery_eis.c
    src/battery_soh_fusion.c
    src/battery_balancer.c
    src/battery_gas_early_warning.c
    src/battery_pulse_heating.c
    src/battery_thermal_mesh.c
    src/battery_sop_dynamic_bounds.c
    src/fall_detection.c
    src/emg_dsp.c
    src/rehab_kinematics.c
    src/motion_wavelet.c
    src/hrv_analyzer.c
    src/ppg_spo2.c
    src/bia_estimator.c
    src/ecg_arrhythmia.c
    src/tremor_analyzer.c
    src/gait_balance.c
    src/apnea_detector.c
    src/dicom_parser.c
    src/federated_aggregator.c
    src/impedance_control.c
    src/gait_safety.c
    src/facility_coordinator.c
    src/edge_cep_engine.c
    src/onnx_host_guard.c
    src/ai_quant_calibrator.c
    src/ai_awq_gptq_gemm.c
    src/ai_temporal_fault_net.c
    src/ai_dynamic_tensor_pool.c
    src/ai_slm_kv_cache.c
    src/ai_graph_pruner.c
    src/ai_fast_gemm.c
    src/ai_pinn_evaluator.c
    src/ai_hetero_pipeline.c
    src/edge_vsg_engine.c
    src/edge_operator_fusion.c
    src/battery_thermal_pinn.c
    src/edge_wasm_nn.c
)

add_library(edge_algo STATIC
    ${ALGO_SOURCES}
)

target_include_directories(edge_algo PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/foundation/include
)
target_link_libraries(edge_algo PUBLIC edge_math libuv::uv_a)
