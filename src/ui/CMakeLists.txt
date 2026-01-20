# src/ui/CMakeLists.txt
add_library(edge_ui_framework INTERFACE)

target_include_directories(edge_ui_framework INTERFACE
    ${CMAKE_SOURCE_DIR}/include
)

target_sources(edge_ui_framework INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/ui_service.c
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/ui_driver_ncurses.c
)
