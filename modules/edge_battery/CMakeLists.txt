add_library(edge_battery STATIC
    src/battery_balancer.c
    src/battery_eis.c
    src/battery_fo_ecm_ekf.c
    src/battery_gas_early_warning.c
    src/battery_micro_isc.c
    src/battery_pulse_heating.c
    src/battery_soh_fusion.c
    src/battery_sop_dynamic_bounds.c
    src/battery_spatial_matrix.c
    src/battery_strategy.c
    src/battery_thermal_mesh.c
    src/battery_thermal_pinn.c
)

target_include_directories(edge_battery PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/foundation/include
)
target_link_libraries(edge_battery PUBLIC edge_math)
