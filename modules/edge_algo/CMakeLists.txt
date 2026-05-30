# modules/edge_algo/CMakeLists.txt

find_package(GSL QUIET)
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
    src/algo_peak_shaving.c
    src/math/control_algo.c
    src/math/motor_algo.c
    src/math/radar_algo.c
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
