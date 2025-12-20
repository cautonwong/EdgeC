# platform/sdk/CMakeLists.txt
#
# Defines the core C-EdgeX SDK library and exports it for consumption
# by other projects.

# Collect sources from sub-modules
add_subdirectory(task_system)
add_subdirectory(utils)

# Define the SDK library target.
# It is a STATIC library, meaning it will be linked directly into the
# final product application executable.
add_library(c-edgex-sdk STATIC
  src/bus.c
  src/core.c
  ${SDK_SOURCES}
)
# For now, sources are placeholders. Add real .c files here as SDK grows.

# Alias for easier use
add_library(cedgex::sdk ALIAS c-edgex-sdk)

# Define the public interface of the SDK.
# Any directory added to INTERFACE_INCLUDE_DIRECTORIES will be automatically
# added to the include path of any target that links against c-edgex-sdk.
target_include_directories(c-edgex-sdk
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include> # Path when installed
)

# Link SDK against its dependencies.
# For now, it has no public dependencies. If the SDK's public headers were
# to include, for example, cJSON.h, then cjson would need to be a PUBLIC dependency.
# Private dependencies are used internally by the SDK's .c files.
target_link_libraries(c-edgex-sdk
  PUBLIC
    # Example: cjson
  PRIVATE
    # Example: some-internal-lib
)

# Apply global compiler options (like ASan) to the SDK
if(ENABLE_ASAN AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_options(c-edgex-sdk PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
  target_link_options(c-edgex-sdk PRIVATE -fsanitize=address)
endif()

# ==============================================================================
# Installation and Exporting
# This is the key part for making the SDK consumable via find_package().
# ==============================================================================

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# 1. Install the library targets (e.g., libc-edgex-sdk.a)
install(TARGETS c-edgex-sdk
  EXPORT CEdgeXSDKTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# 2. Install the public headers
install(DIRECTORY include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# 3. Generate and install the `CEdgexSDKConfig.cmake` file.
# This file tells CMake where to find the installed library and headers.
install(EXPORT CEdgeXSDKTargets
  FILE CEdgeXSDKTargets.cmake
  NAMESPACE cedgex::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cedgex-sdk
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/CEdgexSDKConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/CEdgexSDKConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cedgex-sdk
)

# This allows the project to be found with find_package() from the build directory
# without needing to be installed first, which is great for development.
export(EXPORT CEdgeXSDKTargets
  FILE ${CMAKE_CURRENT_BINARY_DIR}/CEdgexSDKTargets.cmake
  NAMESPACE cedgex::
)

configure_package_config_file(
  "cmake/CEdgexSDKConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/CEdgexSDKConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cedgex-sdk"
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/CEdgexSDKConfig.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cedgex-sdk"
)
