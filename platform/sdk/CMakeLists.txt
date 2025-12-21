# platform/sdk/CMakeLists.txt
#
# Defines the core C-EdgeX SDK library and exports it for consumption
# by other projects.

# platform/sdk/CMakeLists.txt

# This list will be populated by our sub-modules
set(SDK_SOURCES "")

# Collect sources from sub-modules
add_subdirectory(core_runtime)
add_subdirectory(logger)

# Define the SDK library target.
add_library(c-edgex-sdk STATIC
  src/bus.c
  src/core.c
  ${SDK_SOURCES}
)

# Alias for easier use
add_library(cedgex::sdk ALIAS c-edgex-sdk)

# Define the public interface of the SDK.
target_include_directories(c-edgex-sdk
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# The SDK's internal implementation needs to see the headers of its modules.
target_include_directories(c-edgex-sdk
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/core_runtime
    ${CMAKE_CURRENT_SOURCE_DIR}/logger
    ${SDK_PRIVATE_INCLUDES}
)

# Link all external dependencies as PRIVATE. This treats them as
# internal implementation details and solves all export-related issues.
target_link_libraries(c-edgex-sdk
  PRIVATE
    cjson
    cmocka_static
    ccronexpr
    zlog
)

# Apply global compiler options (like ASan) to the SDK
if(ENABLE_ASAN AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_options(c-edgex-sdk PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
  target_link_options(c-edgex-sdk PRIVATE -fsanitize=address)
endif()

# --- Installation and Exporting ---
# This section is temporarily disabled to resolve build configuration errors.
# The logic for creating a findable CMake package can be re-enabled later.
#[[
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS c-edgex-sdk
  EXPORT CEdgeXSDKTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

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
#]]
