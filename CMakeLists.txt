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
set(LOCAL_ROOT "${CMAKE_SOURCE_DIR}/.local/${SYSTEM_NAME_LOWER}-${CMAKE_SYSTEM_PROCESSOR}")
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

# Python3 Interpreter detection (supports system python, venv, and virtual environments)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.venv/bin/python3")
    set(Python3_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/.venv/bin/python3")
endif()
find_package(Python3 COMPONENTS Interpreter REQUIRED)

# ==============================================================================
# 3. Kconfig Configuration System
# ==============================================================================
set(DOTCONFIG_FILE "${CMAKE_BINARY_DIR}/.config")
set(AUTOCONF_HEADER "${CMAKE_BINARY_DIR}/autoconf.h")

if(NOT CONF_FILE)
    set(CONF_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmd/iems/prj.conf")
endif()

find_program(DEFCONFIG_BIN defconfig HINTS "${CMAKE_CURRENT_SOURCE_DIR}/.venv/bin")
find_program(GENCONFIG_BIN genconfig HINTS "${CMAKE_CURRENT_SOURCE_DIR}/.venv/bin")

# If defconfig is not found, check if kconfiglib can be imported or needs bootstrapping
if(NOT DEFCONFIG_BIN)
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -c "import kconfiglib"
        RESULT_VARIABLE kconfiglib_check
        OUTPUT_QUIET ERROR_QUIET
    )
    if(NOT kconfiglib_check EQUAL 0 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/scripts/setup_env.sh")
        message(STATUS "[Kconfig] Bootstrapping Python environment and dependencies...")
        execute_process(
            COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/scripts/setup_env.sh"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            RESULT_VARIABLE setup_res
        )
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.venv/bin/python3")
            set(Python3_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/.venv/bin/python3")
        endif()
        find_program(DEFCONFIG_BIN defconfig HINTS "${CMAKE_CURRENT_SOURCE_DIR}/.venv/bin")
        find_program(GENCONFIG_BIN genconfig HINTS "${CMAKE_CURRENT_SOURCE_DIR}/.venv/bin")
    endif()
endif()

if(DEFCONFIG_BIN)
    set(DEFCONFIG_CMD "${DEFCONFIG_BIN}")
else()
    set(DEFCONFIG_CMD "${Python3_EXECUTABLE}" -m defconfig)
endif()

if(GENCONFIG_BIN)
    set(GENCONFIG_CMD "${GENCONFIG_BIN}")
else()
    set(GENCONFIG_CMD "${Python3_EXECUTABLE}" -m genconfig)
endif()

execute_process(
    COMMAND ${CMAKE_COMMAND} -E env KCONFIG_CONFIG=${DOTCONFIG_FILE} srctree=${CMAKE_CURRENT_SOURCE_DIR}
            ${DEFCONFIG_CMD}
            --kconfig ${CMAKE_CURRENT_SOURCE_DIR}/Kconfig
            ${CONF_FILE}
    RESULT_VARIABLE kconfig_res
)

execute_process(
    COMMAND ${CMAKE_COMMAND} -E env KCONFIG_CONFIG=${DOTCONFIG_FILE} srctree=${CMAKE_CURRENT_SOURCE_DIR}
            ${GENCONFIG_CMD}
            --header-path ${AUTOCONF_HEADER}
            ${CMAKE_CURRENT_SOURCE_DIR}/Kconfig
    RESULT_VARIABLE genconfig_res
)

if(NOT EXISTS "${DOTCONFIG_FILE}" AND EXISTS "${CONF_FILE}")
    # Kconfig unavailable: merge committed default-y options first, then let
    # prj.conf override, so the fallback matches a real `defconfig` run.
    set(FALLBACK_CONFIG_CONTENT "")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/scripts/kconfig_defaults.conf")
        file(READ "${CMAKE_CURRENT_SOURCE_DIR}/scripts/kconfig_defaults.conf" KCONFIG_DEFAULTS_CONTENT)
        set(FALLBACK_CONFIG_CONTENT "${KCONFIG_DEFAULTS_CONTENT}\n")
    endif()
    file(READ "${CONF_FILE}" CONF_FILE_CONTENT)
    string(APPEND FALLBACK_CONFIG_CONTENT "${CONF_FILE_CONTENT}")
    file(WRITE "${DOTCONFIG_FILE}" "${FALLBACK_CONFIG_CONTENT}")
endif()

if(NOT EXISTS "${AUTOCONF_HEADER}")
    message(WARNING "[Kconfig] ${AUTOCONF_HEADER} was not generated; creating fallback header from config")
    set(AUTOCONF_CONTENT "/* Auto-generated fallback autoconf.h */\n#ifndef AUTOCONF_H\n#define AUTOCONF_H\n")
    if(EXISTS "${DOTCONFIG_FILE}")
        file(STRINGS "${DOTCONFIG_FILE}" CFG_LINES)
        foreach(line ${CFG_LINES})
            if(line MATCHES "^CONFIG_([^=]+)=(.*)$")
                set(C_KEY "CONFIG_${CMAKE_MATCH_1}")
                set(C_VAL ${CMAKE_MATCH_2})
                if(C_VAL STREQUAL "y")
                    string(APPEND AUTOCONF_CONTENT "#define ${C_KEY} 1\n")
                elseif(NOT C_VAL STREQUAL "n")
                    string(APPEND AUTOCONF_CONTENT "#define ${C_KEY} ${C_VAL}\n")
                endif()
            endif()
        endforeach()
    endif()
    string(APPEND AUTOCONF_CONTENT "#ifndef CONFIG_EDGEC_TIER\n#define CONFIG_EDGEC_TIER \"ENTERPRISE\"\n#endif\n")
    string(APPEND AUTOCONF_CONTENT "#endif /* AUTOCONF_H */\n")
    file(WRITE "${AUTOCONF_HEADER}" "${AUTOCONF_CONTENT}")
endif()

# Parse .config for CMake usage
if(EXISTS "${DOTCONFIG_FILE}")
    file(STRINGS "${DOTCONFIG_FILE}" CONFIG_LINES)
    foreach(line ${CONFIG_LINES})
        if(line MATCHES "^CONFIG_([^=]+)=(.*)$")
            set(CONF_KEY "CONFIG_${CMAKE_MATCH_1}")
            set(CONF_VAL ${CMAKE_MATCH_2})
            if(CONF_VAL STREQUAL "y")
                set(${CONF_KEY} ON)
                add_definitions(-D${CONF_KEY})
            elseif(CONF_VAL STREQUAL "n")
                set(${CONF_KEY} OFF)
            else()
                set(${CONF_KEY} ${CONF_VAL})
                add_definitions(-D${CONF_KEY}=${CONF_VAL})
            endif()
        elseif(line MATCHES "^# CONFIG_([^ ]+) is not set")
            set(CONF_KEY "CONFIG_${CMAKE_MATCH_1}")
            set(${CONF_KEY} OFF)
        endif()
    endforeach()
else()
    message(WARNING "[Kconfig] ${DOTCONFIG_FILE} was not generated; skipping config parsing")
endif()

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
option(ENABLE_COVERAGE "Enable code coverage reporting" OFF)

target_compile_options(edge_common_interface INTERFACE -Wall -Wextra -Wformat=2 -Wconversion)
if(COVERAGE OR ENABLE_COVERAGE)
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
add_subdirectory(modules/edge_infra)
add_subdirectory(modules/edge_mgmt)

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
add_subdirectory(examples/custom_driver)
add_subdirectory(examples/custom_algo)
enable_testing()
add_subdirectory(tests)

# ==============================================================================
# 7. CPack Packaging Configuration
# ==============================================================================
set(CPACK_PACKAGE_NAME "EdgeOS")
set(CPACK_PACKAGE_VENDOR "EdgeOS Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "EdgeOS Industrial Edge Computing Platform")
set(CPACK_PACKAGE_VERSION_MAJOR 1)
set(CPACK_PACKAGE_VERSION_MINOR 0)
set(CPACK_PACKAGE_VERSION_PATCH 0)
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_GENERATOR "TGZ;DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "EdgeOS Architect <dev@edgeos.io>")
set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
include(CPack)
