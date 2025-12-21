# C-EdgeX Project - Top-Level CMakeLists.txt
#
# This file is the main entry point for the build system.
# It sets up project-wide settings and includes the platform and product builds.

cmake_minimum_required(VERSION 3.22)

project(
  C-EdgeX-Project
  VERSION 0.1.0
  DESCRIPTION "A Professional C Boilerplate for Edge Computing"
  LANGUAGES C
)

# ==============================================================================
# Global Settings & Options
# ==============================================================================

# Set a consistent C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_definitions(_GNU_SOURCE)

# Add compiler warnings for high-quality code
add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wmissing-prototypes -Wstrict-prototypes)

# Define a build option for enabling AddressSanitizer (ASan)
# This will be controlled via CMake Presets.
option(ENABLE_ASAN "Enable AddressSanitizer for memory safety" ON)

# ==============================================================================
# Project Structure
# ==============================================================================

# Add the platform, which contains the SDK and all core components.
# This will be built first, and it will export its targets.
add_subdirectory(platform)

# Add the platform's binary directory to the search path for packages.
# This allows find_package(CEdgexSDK) to work from the build tree.
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR}/platform/sdk)

# Add the product applications.
# Each application in this directory will be a separate executable
# that consumes the platform SDK.
add_subdirectory(products/sim_sensor_app)
