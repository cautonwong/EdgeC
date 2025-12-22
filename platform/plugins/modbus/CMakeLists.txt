# platform/plugins/modbus/CMakeLists.txt

add_library(modbus_driver SHARED
    src/modbus_drv.c
)

# Link against the libraries it needs
target_link_libraries(modbus_driver PRIVATE
    cedgex::sdk
    modbus
    cjson
)

# Install the shared library to the 'plugins' directory
install(TARGETS modbus_driver
    LIBRARY DESTINATION lib/cedgex/plugins
)