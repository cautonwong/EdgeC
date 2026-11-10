add_library(edge_power STATIC
    src/edge_vsg_engine.c
    src/facility_coordinator.c
    src/vsg_black_start.c
    src/vsg_circulating.c
    src/vsg_compute_tracking.c
    src/vsg_control.c
    src/vsg_frt.c
    src/vsg_presync.c
    src/vsg_sso.c
    src/vsg_virtual_z.c
)

target_include_directories(edge_power PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/foundation/include
)
target_include_directories(edge_power PRIVATE
    src
)
target_link_libraries(edge_power PUBLIC edge_math)
