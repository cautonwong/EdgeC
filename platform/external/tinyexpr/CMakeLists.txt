# platform/external/tinyexpr/CMakeLists.txt

add_library(tinyexpr STATIC
    tinyexpr.c
    tinyexpr.h
)

target_include_directories(tinyexpr PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
