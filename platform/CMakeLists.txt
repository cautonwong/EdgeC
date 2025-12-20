# platform/CMakeLists.txt
#
# This file orchestrates the build of all components within the "platform" domain.
# It includes third-party libraries and the main C-EdgeX SDK.

# 1. Include third-party libraries
# These are treated as internal components of the platform.
add_subdirectory(external/cJSON)

# Disable building examples and tests for cmocka, as we only need the library.
set(WITH_EXAMPLES OFF CACHE BOOL "Disable cmocka examples")
set(WITH_TESTS OFF CACHE BOOL "Disable cmocka internal tests")
add_subdirectory(external/cmocka)

# 2. Include the main C-EdgeX SDK
# This is the core library that will be exported for product applications to use.
add_subdirectory(sdk)

# 3. Include tests for the platform
# Tests are considered part of the platform's development and verification.
add_subdirectory(tests)
