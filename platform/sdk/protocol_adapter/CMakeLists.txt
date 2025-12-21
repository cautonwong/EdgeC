# platform/sdk/protocol_adapter/CMakeLists.txt

list(APPEND SDK_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/plugin_manager.c
  ${CMAKE_CURRENT_SOURCE_DIR}/metadata_manager.c
)

list(APPEND SDK_PRIVATE_INCLUDES
  ${CMAKE_CURRENT_SOURCE_DIR}
)
