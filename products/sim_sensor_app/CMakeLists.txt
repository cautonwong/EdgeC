# products/sim_sensor_app/CMakeLists.txt
#
# This file defines a specific product application that consumes the C-EdgeX SDK.

add_executable(sim_sensor_app
  main.c
  sim_driver.c
)

# --- Find Dependencies ---

# In a monolithic build like this boilerplate, the `cedgex::sdk` target is
# already known to CMake because we added the `platform` directory in the top-level
# CMakeLists.txt. We can therefore link to it directly.
#
# In a true multi-repo setup (as described in manifest.xml), the product app
# would be in a separate repository. In that case, you would uncomment the
# following line and use `find_package` to locate the pre-built SDK.
# find_package(CEdgexSDK REQUIRED)

# Use pkg-config to find system-installed libraries like libuv. This is generally
# more reliable on Linux systems.
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUV REQUIRED libuv)

# Find the standard pthreads library.
find_package(Threads REQUIRED)


# --- Link Dependencies ---

target_link_libraries(sim_sensor_app
  PRIVATE
    cedgex::sdk           # Link against our internal SDK
    ${LIBUV_LIBRARIES}    # Link against libuv
    Threads::Threads      # Link against pthreads
)

target_include_directories(sim_sensor_app
  PRIVATE
    ${LIBUV_INCLUDE_DIRS}
)

target_compile_definitions(sim_sensor_app PRIVATE _GNU_SOURCE)


# --- Set Target Properties ---

set_target_properties(sim_sensor_app
  PROPERTIES
    OUTPUT_NAME "sim_sensor_app"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Apply global compiler options (like ASan)
if(ENABLE_ASAN AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_options(sim_sensor_app PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
  target_link_options(sim_sensor_app PRIVATE -fsanitize=address)
endif()

target_include_directories(sim_sensor_app
  PRIVATE
    ${LIBUV_INCLUDE_DIRS}
)
