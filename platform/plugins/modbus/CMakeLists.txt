# platform/plugins/modbus/CMakeLists.txt

project(modbus_driver C)

# Create a shared library from the driver source code
add_library(modbus_driver MODULE
    src/modbus_drv.c
)

# The driver needs to know about the SDK's public API
# and also the internal API of the protocol adapter.
target_include_directories(modbus_driver PRIVATE
    ${CMAKE_SOURCE_DIR}/platform/sdk/include
    ${CMAKE_SOURCE_DIR}/platform/sdk/protocol_adapter
)

# Link against libmodbus, which it uses internally.
# cjson is also needed for parsing the config string.
target_link_libraries(modbus_driver PRIVATE
    modbus
    cjson
)

# All plugins should be placed in a 'plugins' directory in the build output
set_target_properties(modbus_driver PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
)
