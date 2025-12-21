# platform/sdk/storage/CMakeLists.txt

list(APPEND SDK_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/db_engine.c
)

list(APPEND SDK_PRIVATE_INCLUDES
  ${CMAKE_CURRENT_SOURCE_DIR}
)
