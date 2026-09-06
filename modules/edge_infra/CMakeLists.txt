# ==============================================================================
# EdgeOS Infrastructure & Driver Subsystem (edge_infra)
# ==============================================================================

set(INFRA_SOURCES
    src/edge_infra_base.c
    src/edge_anti_corruption.c
    storage/edge_telemetry_store.c
)
set(INFRA_PUBLIC_LIBS edge_common_interface)
set(INFRA_PRIVATE_LIBS libuv::uv_a zlog::zlog)

# Storage: SQLite Service & Driver (Kconfig: CONFIG_EDGE_INFRA_SQLITE)
if(CONFIG_EDGE_INFRA_SQLITE)
    list(APPEND INFRA_SOURCES
        storage/sqlite_service.c
        storage/sqlite_storage_driver.c
    )
    list(APPEND INFRA_PRIVATE_LIBS SQLite::SQLite3)
endif()

# Distributed Bus: CycloneDDS Backend (Kconfig: CONFIG_EDGE_INFRA_DDS)
if(CONFIG_EDGE_INFRA_DDS)
    list(APPEND INFRA_SOURCES
        bus_dds/datahub_dds.c
        bus_dds/dds_topology_monitor.c
    )
    list(APPEND INFRA_PUBLIC_LIBS cyclonedds)
endif()

# Northbound Bus: MQTT Bridge Backend (Kconfig: CONFIG_EDGE_INFRA_MQTT)
if(CONFIG_EDGE_INFRA_MQTT)
    list(APPEND INFRA_SOURCES
        bus_mqtt/datahub_mqtt.c
    )
    list(APPEND INFRA_PRIVATE_LIBS PahoMQTT::PahoMQTT)
endif()

# ==============================================================================
# IDL Code Generation & Header Preparation
# ==============================================================================
set(IDL_GEN_DIR "${CMAKE_BINARY_DIR}/idl_generated")
file(MAKE_DIRECTORY "${IDL_GEN_DIR}")

find_program(IDLC_BIN idlc HINTS "${LOCAL_ROOT}/bin")
if(IDLC_BIN AND EXISTS "${CMAKE_SOURCE_DIR}/contracts/idl/dlt698_objects.idl")
    execute_process(
        COMMAND "${IDLC_BIN}" -o "${IDL_GEN_DIR}" "${CMAKE_SOURCE_DIR}/contracts/idl/dlt698_objects.idl"
        OUTPUT_QUIET ERROR_QUIET
        RESULT_VARIABLE idlc_res
    )
endif()

if(NOT EXISTS "${IDL_GEN_DIR}/dlt698_objects.h" AND EXISTS "${CMAKE_SOURCE_DIR}/contracts/idl/generated/dlt698_objects.h")
    file(COPY "${CMAKE_SOURCE_DIR}/contracts/idl/generated/dlt698_objects.h" DESTINATION "${IDL_GEN_DIR}")
    file(COPY "${CMAKE_SOURCE_DIR}/contracts/idl/generated/dlt698_objects.c" DESTINATION "${IDL_GEN_DIR}")
endif()

add_library(edge_infra STATIC
    ${INFRA_SOURCES}
)

target_include_directories(edge_infra PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
    ${IDL_GEN_DIR}
    ${CMAKE_SOURCE_DIR}/contracts/idl/generated
)

target_link_libraries(edge_infra PUBLIC ${INFRA_PUBLIC_LIBS})
target_link_libraries(edge_infra PRIVATE ${INFRA_PRIVATE_LIBS})
