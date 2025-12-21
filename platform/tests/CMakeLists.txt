# platform/tests/CMakeLists.txt
#
# This file defines the test runners for the platform SDK.

# Enable CTest
enable_testing()

# Find dependencies for the test runner
find_package(Threads REQUIRED)

# Add the test runner executable
add_executable(sdk_tests
  test_device_interface.c
  ../../products/sim_sensor_app/sim_driver.c
)

# Link the test runner against:
# 1. The SDK we are testing (cedgex::sdk)
# 2. The cmocka testing framework (cmocka)
# 3. pthreads, which cmocka may require on some platforms
target_link_libraries(sdk_tests
  PRIVATE
    cedgex::sdk
    cmocka_static
    Threads::Threads
)

# Apply global compiler options (like ASan)
if(ENABLE_ASAN AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_options(sdk_tests PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
  target_link_options(sdk_tests PRIVATE -fsanitize=address)
endif()

# Register the executable with CTest, so `ctest` can run it.
add_test(NAME Platform.SDK.Tests COMMAND sdk_tests)
