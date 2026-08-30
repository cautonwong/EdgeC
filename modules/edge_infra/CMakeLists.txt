# ==============================================================================
# EdgeOS Infrastructure & Driver Subsystem (edge_infra)
# ==============================================================================

set(INFRA_SOURCES
    src/edge_infra_base.c
    src/edge_anti_corruption.c
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

add_library(edge_infra STATIC
    ${INFRA_SOURCES}
)

target_include_directories(edge_infra PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)

target_link_libraries(edge_infra PUBLIC ${INFRA_PUBLIC_LIBS})
target_link_libraries(edge_infra PRIVATE ${INFRA_PRIVATE_LIBS})
