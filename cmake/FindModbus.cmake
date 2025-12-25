# - Find Modbus
# This module finds an installed Modbus library.
#
# It sets the following variables:
#  MODBUS_FOUND            - True if Modbus was found.
#  MODBUS_INCLUDE_DIRS     - The Modbus include directories.
#  MODBUS_LIBRARIES        - The Modbus libraries.

find_path(MODBUS_INCLUDE_DIR modbus.h
    HINTS ${CMAKE_PREFIX_PATH}/include
    PATH_SUFFIXES include include/modbus)

find_library(MODBUS_LIBRARY NAMES modbus
    HINTS ${CMAKE_PREFIX_PATH}/lib
    PATH_SUFFIXES lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Modbus DEFAULT_MSG
    MODBUS_LIBRARY MODBUS_INCLUDE_DIR)

if(MODBUS_FOUND)
    set(MODBUS_LIBRARIES ${MODBUS_LIBRARY})
    set(MODBUS_INCLUDE_DIRS ${MODBUS_INCLUDE_DIR})
    
    if(NOT TARGET Modbus::Modbus)
        add_library(Modbus::Modbus UNKNOWN IMPORTED)
        set_target_properties(Modbus::Modbus PROPERTIES
            IMPORTED_LOCATION "${MODBUS_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${MODBUS_INCLUDE_DIR}")
        
        # libmodbus requires linking against pthread on POSIX systems
        if(UNIX AND NOT APPLE)
            target_link_libraries(Modbus::Modbus INTERFACE Threads::Threads)
        endif()
    endif()
endif()

mark_as_advanced(MODBUS_INCLUDE_DIR MODBUS_LIBRARY)
