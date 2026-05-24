# modules/edge_algo/CMakeLists.txt

find_package(GSL QUIET)
if(GSL_FOUND)
    set(MATH_IMPL src/math/linalg_gsl.c)
    set(MATH_DEFS USE_GSL)
else()
    set(MATH_IMPL src/math/linalg_builtin.c)
    set(MATH_DEFS USE_BUILTIN_MATH)
endif()

add_library(edge_algo STATIC
    src/algorithm_sum.c
    src/battery_strategy.c
    src/algo_peak_shaving.c
    src/math/windowing.c
    src/math/dsp_algo.c
    src/math/control_algo.c
    src/math/motor_algo.c
    src/math/radar_algo.c
    src/math/quat_math.c
    ${MATH_IMPL}
)

target_include_directories(edge_algo PUBLIC include)
target_link_libraries(edge_algo PUBLIC edge_core)

if(GSL_FOUND)
    target_link_libraries(edge_algo PRIVATE GSL::gsl GSL::gslcblas)
endif()

target_compile_definitions(edge_algo PRIVATE ${MATH_DEFS})
