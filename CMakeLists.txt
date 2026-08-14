cmake_minimum_required(VERSION 3.16)
project(EdgeC C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# [Build Optimization] Enable ccache if available
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    message(STATUS "[Build-Opt] Enabled ccache for compilation speedup")
endif()

# [Build Optimization] Option to bypass HMI web frontend builds
option(ENABLE_WEB_BUILD "Enable frontend Web UI compilation" ON)

# ==============================================================================
# 1. Environment & Path Configuration
# ==============================================================================
string(TOLOWER "${CMAKE_HOST_SYSTEM_NAME}" SYSTEM_NAME_LOWER)
set(LOCAL_ROOT "${CMAKE_SOURCE_DIR}/.local/${SYSTEM_NAME_LOWER}-${CMAKE_HOST_SYSTEM_PROCESSOR}")
list(APPEND CMAKE_PREFIX_PATH "${LOCAL_ROOT}")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# ==============================================================================
# 2. Find Dependencies
# ==============================================================================
find_package(libuv REQUIRED)
find_package(cJSON REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(zlog REQUIRED)
find_package(PahoMQTT REQUIRED)
find_package(CURL REQUIRED)
find_package(QuickJS REQUIRED)
find_package(iwasm REQUIRED)
find_package(cmocka REQUIRED)

# ==============================================================================
# 3. Kconfig Configuration System
# ==============================================================================
set(DOTCONFIG_FILE "${CMAKE_BINARY_DIR}/.config")
set(AUTOCONF_HEADER "${CMAKE_BINARY_DIR}/autoconf.h")

if(NOT CONF_FILE)
    set(CONF_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmd/iems/prj.conf")
endif()

execute_process(
    COMMAND ${CMAKE_COMMAND} -E env KCONFIG_CONFIG=${DOTCONFIG_FILE} srctree=${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/.venv/bin/python3 ${CMAKE_CURRENT_SOURCE_DIR}/.venv/bin/defconfig
            --kconfig ${CMAKE_CURRENT_SOURCE_DIR}/Kconfig
            ${CONF_FILE}
    RESULT_VARIABLE kconfig_res
)

execute_process(
    COMMAND ${CMAKE_COMMAND} -E env KCONFIG_CONFIG=${DOTCONFIG_FILE} srctree=${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/.venv/bin/python3 ${CMAKE_CURRENT_SOURCE_DIR}/.venv/bin/genconfig
            --header-path ${AUTOCONF_HEADER}
            ${CMAKE_CURRENT_SOURCE_DIR}/Kconfig
)

# Parse .config for CMake usage
file(STRINGS "${DOTCONFIG_FILE}" CONFIG_LINES)
foreach(line ${CONFIG_LINES})
    if(line MATCHES "^CONFIG_([^=]+)=(.*)$")
        set(CONF_KEY "CONFIG_${CMAKE_MATCH_1}")
        set(CONF_VAL ${CMAKE_MATCH_2})
        if(CONF_VAL STREQUAL "y")
            set(${CONF_KEY} ON)
            add_definitions(-D${CONF_KEY})
        else()
            set(${CONF_KEY} ${CONF_VAL})
            add_definitions(-D${CONF_KEY}=${CONF_VAL})
        endif()
    endif()
endforeach()

# ==============================================================================
# 4. Common Build Options (Interface Target)
# ==============================================================================
add_library(edge_common_interface INTERFACE)
target_include_directories(edge_common_interface INTERFACE 
    include 
    "${CMAKE_BINARY_DIR}"
    SYSTEM "${LOCAL_ROOT}/include"
)
target_compile_definitions(edge_common_interface INTERFACE _DEFAULT_SOURCE)
option(COVERAGE "Enable code coverage reporting" OFF)

target_compile_options(edge_common_interface INTERFACE -Wall -Wextra -Wformat=2 -Wconversion)
if(COVERAGE)
    target_compile_options(edge_common_interface INTERFACE --coverage)
    target_link_options(edge_common_interface INTERFACE --coverage)
endif()

option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)

if(ENABLE_ASAN)
    target_compile_options(edge_common_interface INTERFACE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(edge_common_interface INTERFACE -fsanitize=address)
endif()

if(ENABLE_TSAN)
    target_compile_options(edge_common_interface INTERFACE -fsanitize=thread)
    target_link_options(edge_common_interface INTERFACE -fsanitize=thread)
endif()


# ==============================================================================
# 5. Modular Packages
# ==============================================================================
add_subdirectory(third-party)
add_subdirectory(modules/edge_core)
add_subdirectory(modules/edge_algo)
add_subdirectory(modules/edge_protocol)
add_subdirectory(modules/edge_security)
add_subdirectory(modules/edge_stream)
add_subdirectory(modules/edge_io)

# ==============================================================================
# 6. Applications & Tests
# ==============================================================================
include(EdgeContract)
include(WebBuild)
include(EdgeInstall)
include(BuildContractCompiler)

build_contract_compiler()

add_subdirectory(cmd/smart_box)
add_subdirectory(cmd/iems)
add_subdirectory(cmd/dcu)
add_subdirectory(cmd/nexus_lab)
enable_testing()
add_subdirectory(tests)

# Helper for test registration
