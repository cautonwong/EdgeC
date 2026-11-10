add_library(edge_health STATIC
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
    src/gait_safety.c
    src/impedance_control.c
)

target_include_directories(edge_health PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/foundation/include
)
target_link_libraries(edge_health PUBLIC edge_math)
