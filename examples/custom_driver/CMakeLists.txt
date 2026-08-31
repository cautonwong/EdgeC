# Example: Custom Inverter Driver Standalone Library
add_library(custom_inverter_driver STATIC
    src/custom_inverter_driver.c
)

target_include_directories(custom_inverter_driver PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/modules/edge_infra/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
    ${CMAKE_SOURCE_DIR}/modules/edge_io/include
)

target_link_libraries(custom_inverter_driver PRIVATE
    edge_common_interface
)
