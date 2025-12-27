# - Find PahoMQTT
# This module finds the Paho MQTT C library.
#
# It sets the following variables:
#  PAHOMQTT_FOUND            - True if PahoMQTT was found.
#  PAHOMQTT_INCLUDE_DIRS     - The PahoMQTT include directories.
#  PAHOMQTT_LIBRARIES        - The PahoMQTT libraries.

find_path(PAHOMQTT_INCLUDE_DIR MQTTAsync.h
    HINTS ${CMAKE_PREFIX_PATH}
    PATH_SUFFIXES include)

# Paho has two main versions: Async and Sync. Usually we want Async for this project.
find_library(PAHOMQTT_AS_LIBRARY NAMES paho-mqtt3a
    HINTS ${CMAKE_PREFIX_PATH}
    PATH_SUFFIXES lib)

find_library(PAHOMQTT_C_LIBRARY NAMES paho-mqtt3c
    HINTS ${CMAKE_PREFIX_PATH}
    PATH_SUFFIXES lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PahoMQTT DEFAULT_MSG
    PAHOMQTT_AS_LIBRARY PAHOMQTT_INCLUDE_DIR)

if(PAHOMQTT_FOUND)
    set(PAHOMQTT_LIBRARIES ${PAHOMQTT_AS_LIBRARY})
    set(PAHOMQTT_INCLUDE_DIRS ${PAHOMQTT_INCLUDE_DIR})
    
    if(NOT TARGET PahoMQTT::PahoMQTT)
        add_library(PahoMQTT::PahoMQTT UNKNOWN IMPORTED)
        set_target_properties(PahoMQTT::PahoMQTT PROPERTIES
            IMPORTED_LOCATION "${PAHOMQTT_AS_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${PAHOMQTT_INCLUDE_DIR}")
        
        if(UNIX)
            set_property(TARGET PahoMQTT::PahoMQTT APPEND PROPERTY INTERFACE_LINK_LIBRARIES "pthread;dl")
        endif()
    endif()
endif()

mark_as_advanced(PAHOMQTT_INCLUDE_DIR PAHOMQTT_AS_LIBRARY PAHOMQTT_C_LIBRARY)
