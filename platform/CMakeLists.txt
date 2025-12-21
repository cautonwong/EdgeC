# platform/CMakeLists.txt
#
# This file orchestrates the build of all components within the "platform" domain.
# It includes third-party libraries and the main C-EdgeX SDK.

# 1. Include third-party libraries
# These are treated as internal components of the platform.
add_subdirectory(external/cJSON)
add_subdirectory(external/cmocka)
add_subdirectory(external/ccronexpr)
add_subdirectory(external/zlog)
add_subdirectory(external/sqlite)
add_subdirectory(external/tinyexpr)
add_subdirectory(external/libmodbus)

# Include the main C-EdgeX SDK
add_subdirectory(sdk)

# Include tests for the platform
add_subdirectory(tests)

# Include plugins
add_subdirectory(plugins/modbus)
