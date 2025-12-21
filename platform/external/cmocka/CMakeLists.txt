# Simplified CMakeLists.txt for vendored cmocka

# The zlog submodule also has a file named 'src', which can confuse CMake.
# We set this property to prevent that.
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY CMAKE_DIRECTORY_ID "cmocka_id")

# This library is built from its own source files, not by adding a subdirectory
add_library(cmocka_static STATIC
    src/cmocka.c
)

# Add the necessary compile definition to make POSIX functions visible.
# This is critical to fixing the compilation errors.
target_compile_definitions(cmocka_static PRIVATE _GNU_SOURCE)

# Set the public include directory, which contains cmocka.h
target_include_directories(cmocka_static PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# For the implementation, it also needs its own 'src' directory
# for private headers like cmocka_private.h
target_include_directories(cmocka_static PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
