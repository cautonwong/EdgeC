# modules/edge_algo/CMakeLists.txt

if(NOT CMAKE_CROSSCOMPILING)
    find_package(GSL QUIET)
endif()
if(GSL_FOUND)
    set(MATH_IMPL src/math/linalg_gsl.c)
    set(MATH_DEFS USE_GSL)
else()
    set(MATH_IMPL src/math/linalg_builtin.c)
    set(MATH_DEFS USE_BUILTIN_MATH)
endif()

set(ALGO_SOURCES
    src/algorithm_sum.c
    src/battery_strategy.c
    src/battery_fo_ecm_ekf.c
    src/battery_micro_isc.c
    src/battery_spatial_matrix.c
    src/battery_eis.c
    src/algo_peak_shaving.c
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
    src/math/control_algo.c
    src/math/motor_algo.c
    src/math/radar_algo.c
    src/math/fmcw_dsp_neon.c
)

# 根据 Kconfig 条件加载高精度数值与矩阵库 (以接地气的中文添加注释)
if(CONFIG_EDGEC_ALGO_LINALG)
    list(APPEND ALGO_SOURCES ${MATH_IMPL})
endif()

# 根据 Kconfig 条件加载信号处理与窗函数算法
if(CONFIG_EDGEC_ALGO_DSP)
    list(APPEND ALGO_SOURCES
        src/math/windowing.c
        src/math/dsp_algo.c
    )
endif()

# 根据 Kconfig 条件加载 AHRS 姿态融合四元数算法
if(CONFIG_EDGEC_ALGO_AHRS)
    list(APPEND ALGO_SOURCES src/math/quat_math.c)
endif()

add_library(edge_algo STATIC
    ${ALGO_SOURCES}
)

target_include_directories(edge_algo PUBLIC include)
target_link_libraries(edge_algo PUBLIC edge_core)

if(GSL_FOUND)
    target_link_libraries(edge_algo PRIVATE GSL::gsl GSL::gslcblas)
endif()

target_compile_definitions(edge_algo PRIVATE ${MATH_DEFS})
