add_library(mqtt_transport STATIC
    src/mqtt_producer.c
)

target_include_directories(mqtt_transport PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/modules/edge_north/include
    ${CMAKE_SOURCE_DIR}/foundation/include
)

target_link_libraries(mqtt_transport PUBLIC edge_north PahoMQTT::PahoMQTT)
