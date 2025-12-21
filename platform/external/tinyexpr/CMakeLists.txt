# platform/external/tinyexpr/CMakeLists.txt

add_library(tinyexpr STATIC
    tinyexpr.c
)

target_include_directories(tinyexpr PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# This library has many warnings that would fail with our -Werror flag.
target_compile_options(tinyexpr PRIVATE -Wno-error)