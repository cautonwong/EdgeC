# adapters/io_drivers/CMakeLists.txt
#
# Southbound driver adapters. edge_io keeps orchestration, scheduling and
# management; concrete protocol drivers live here and register through the
# driver manager.
add_library(edge_io_drivers STATIC
    src/adc_driver.c
    src/driver_can.c
    src/driver_ccsds.c
    src/driver_cia402.c
    src/driver_dlms.c
    src/driver_dlt645.c
    src/driver_dlt698.c
    src/driver_hl7_fhir.c
    src/driver_hotplug.c
    src/driver_manager.c
    src/driver_iec101.c
    src/driver_iec104.c
    src/driver_mqtt_json.c
    src/driver_radar_fmcw.c
    src/driver_sparkplug_b.c
    src/modbus_rtu_driver.c
    src/modbus_tcp_driver.c
)

target_include_directories(edge_io_drivers PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/modules/edge_io/include
)
target_link_libraries(edge_io_drivers PUBLIC
    edge_core
    edge_codec
    edge_log
    edge_common_interface
)
target_link_libraries(edge_io_drivers PRIVATE cjson libuv::uv_a)
