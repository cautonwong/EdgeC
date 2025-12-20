# cmake/arm64-gnu.toolchain.cmake
#
# This file guides CMake on how to cross-compile for an ARM64 target (aarch64-linux-gnu).
# You may need to adjust the paths and compiler names based on the toolchain
# installed on your system or in your Docker build environment.

# Target system definition
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Find the cross-compilers
# The "aarch64-linux-gnu-" prefix might change depending on your toolchain.
# The `which` command is used to find the compiler in the system PATH.
# This is useful for CI environments where the toolchain is installed in a standard location.
execute_process(
  COMMAND which aarch64-linux-gnu-gcc
  OUTPUT_VARIABLE CMAKE_C_COMPILER
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND which aarch64-linux-gnu-g++
  OUTPUT_VARIABLE CMAKE_CXX_COMPILER
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# If the compilers are not found, you can uncomment and set them manually:
# if(NOT CMAKE_C_COMPILER)
#   set(CMAKE_C_COMPILER /usr/bin/aarch64-linux-gnu-gcc)
# endif()
# if(NOT CMAKE_CXX_COMPILER)
#   set(CMAKE_CXX_COMPILER /usr/bin/aarch64-linux-gnu-g++)
# endif()

# Define where to find headers and libraries for the target system.
# CMAKE_FIND_ROOT_PATH points to the sysroot of the cross-compiler.
# This is usually the directory containing `usr/include`, `usr/lib`, etc.
if(DEFINED ENV{CMAKE_SYSROOT})
  set(CMAKE_SYSROOT $ENV{CMAKE_SYSROOT})
else()
  # If no environment variable is set, try to guess the sysroot from the compiler path.
  if(CMAKE_C_COMPILER)
    execute_process(
      COMMAND ${CMAKE_C_COMPILER} -print-sysroot
      OUTPUT_VARIABLE CMAKE_SYSROOT
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  endif()
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
