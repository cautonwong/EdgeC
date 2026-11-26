# P6-04b assembly guard:
#   1. EDGE_CONFIG_DIR set to a bad directory -> binary refuses to boot, stderr
#      names the config failure and the assembly failure.
#   2. EDGE_CONFIG_DIR unset/empty -> no config load failure; boot reaches the
#      southbound stage.
# The unset run points OMNIS_BUS_*_DEV at missing paths so both static buses
# fail to connect deterministically; it never deletes shared vbus sockets.

file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${WORK}/bad_config" "${WORK}/db")
string(RANDOM LENGTH 10 ALPHABET "0123456789abcdef" run_id)

set(base_env
    "EDGE_REST_PORT=19091"
    "EDGE_REST_BIND=127.0.0.1"
    "EDGE_WAL_PATH=${WORK}/db/wal.log"
    "EDGE_SNAP_PATH=${WORK}/db/snapshot.bin"
)

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
            ${base_env}
            "EDGE_DB_PATH=${WORK}/db/bad.db"
            "EDGE_SHM_NAME=iems_cfg_bad_${run_id}"
            "EDGE_CONFIG_DIR=${WORK}/bad_config"
            "${IEMS_BIN}"
    RESULT_VARIABLE bad_rc
    OUTPUT_VARIABLE bad_out
    ERROR_VARIABLE bad_err
    TIMEOUT 15)
set(bad_text "${bad_out}\n${bad_err}")
if(bad_rc EQUAL 0)
    message(FATAL_ERROR "bad config dir booted successfully; expected refusal\n${bad_text}")
endif()
if(NOT bad_text MATCHES "config load failed")
    message(FATAL_ERROR "bad config boot did not report a config load failure\n${bad_text}")
endif()
if(NOT bad_text MATCHES "Failed to assemble IEMS application")
    message(FATAL_ERROR "bad config boot did not reach the assembly failure path\n${bad_text}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
            ${base_env}
            "EDGE_DB_PATH=${WORK}/db/unset.db"
            "EDGE_SHM_NAME=iems_cfg_unset_${run_id}"
            "EDGE_CONFIG_DIR="
            "OMNIS_BUS_0_DEV=${WORK}/missing0.sock"
            "OMNIS_BUS_1_DEV=${WORK}/missing1.sock"
            "${IEMS_BIN}"
    RESULT_VARIABLE unset_rc
    OUTPUT_VARIABLE unset_out
    ERROR_VARIABLE unset_err
    TIMEOUT 15)
set(unset_text "${unset_out}\n${unset_err}")
if(unset_text MATCHES "config load failed")
    message(FATAL_ERROR "unset EDGE_CONFIG_DIR still attempted a config load\n${unset_text}")
endif()
if(NOT unset_text MATCHES "southbound bus\\(es\\) configured, none connected")
    message(FATAL_ERROR "unset EDGE_CONFIG_DIR did not reach the southbound stage\n${unset_text}")
endif()
if(unset_rc EQUAL 0)
    message(FATAL_ERROR "unset EDGE_CONFIG_DIR boot should fail when both buses are missing\n${unset_text}")
endif()

message(STATUS "assembly guard OK")
