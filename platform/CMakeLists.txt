# platform/CMakeLists.txt
# This file orchestrates the build of all components within the "platform" domain.

# --- 1. Define External Libraries from Vendored Source ---
# By defining them here, we have full control over their build process,
# ensuring they are compiled with -fPIC and are linked correctly.

# cJSON
add_library(cjson STATIC
    external/cJSON/cJSON.c
)
target_include_directories(cjson PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/cJSON>
)
set_target_properties(cjson PROPERTIES POSITION_INDEPENDENT_CODE ON)

# cmocka
add_library(cmocka STATIC
    external/cmocka/src/cmocka.c
)
target_include_directories(cmocka PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/cmocka/include>
)
target_include_directories(cmocka PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/cmocka/src>
)
set_target_properties(cmocka PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_options(cmocka PRIVATE -include signal.h)

# ccronexpr
add_library(ccronexpr STATIC
    external/ccronexpr/ccronexpr.c
)
target_include_directories(ccronexpr PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/ccronexpr>
)
set_target_properties(ccronexpr PROPERTIES POSITION_INDEPENDENT_CODE ON)

# zlog
add_library(zlog STATIC
    external/zlog/src/zlog.c
    external/zlog/src/buf.c
    external/zlog/src/category_table.c
    external/zlog/src/category.c
    external/zlog/src/conf.c
    external/zlog/src/event.c
    external/zlog/src/format.c
    external/zlog/src/level_list.c
    external/zlog/src/level.c
    external/zlog/src/mdc.c
    external/zlog/src/record_table.c
    external/zlog/src/record.c
    external/zlog/src/rotater.c
    external/zlog/src/rule.c
    external/zlog/src/spec.c
    external/zlog/src/thread.c
    external/zlog/src/zc_arraylist.c
    external/zlog/src/zc_profile.c
    external/zlog/src/zc_util.c
    external/zlog/src/zc_hashtable.c
)
target_include_directories(zlog PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/zlog/src>
)
set_target_properties(zlog PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(zlog PRIVATE Threads::Threads)

# libmodbus
add_library(modbus STATIC
    external/libmodbus/src/modbus.c
    external/libmodbus/src/modbus-data.c
    external/libmodbus/src/modbus-rtu.c
    external/libmodbus/src/modbus-tcp.c
)
target_include_directories(modbus PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/libmodbus/src>
)
set_target_properties(modbus PROPERTIES POSITION_INDEPENDENT_CODE ON)

# tinyexpr
add_library(tinyexpr STATIC
    external/tinyexpr/tinyexpr.c
)
target_include_directories(tinyexpr PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/tinyexpr>
)
set_target_properties(tinyexpr PROPERTIES POSITION_INDEPENDENT_CODE ON)

# sqlite
add_library(sqlite STATIC
    external/sqlite/sqlite3.c
)
target_include_directories(sqlite PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/sqlite>
)
set_target_properties(sqlite PROPERTIES POSITION_INDEPENDENT_CODE ON)


# --- 2. Include the main C-EdgeX SDK ---
# The SDK will link against the libraries defined above.
add_subdirectory(sdk)

# --- 3. Include tests for the platform ---
# Tests are considered part of the platform's development and verification.
add_subdirectory(tests)