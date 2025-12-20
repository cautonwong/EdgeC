# platform/sdk/task_system/CMakeLists.txt

# This module currently has no public headers, only internal implementation
# that will be part of the larger c-edgex-sdk library.
# The public interface would be a higher-level API in the SDK's main include dir.

# The source files are added to the parent's target variable.
# This is a common pattern for internal modules.
list(APPEND SDK_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/scheduler.c
  ${CMAKE_CURRENT_SOURCE_DIR}/worker_pool.c
)

# If this module had its own public headers, we would add:
# target_include_directories(c-edgex-sdk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
