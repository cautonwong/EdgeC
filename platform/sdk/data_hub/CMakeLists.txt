# platform/sdk/data_hub/CMakeLists.txt

list(APPEND SDK_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/shm_ring.c
  ${CMAKE_CURRENT_SOURCE_DIR}/tag_manager.c
)

list(APPEND SDK_PRIVATE_INCLUDES
  ${CMAKE_CURRENT_SOURCE_DIR}
)
