# - Find CURL
# This module finds an installed CURL library.
#
# It sets the following variables:
#  CURL_FOUND            - True if CURL was found.
#  CURL_INCLUDE_DIRS     - The CURL include directories.
#  CURL_LIBRARIES        - The CURL libraries.

find_path(CURL_INCLUDE_DIR curl/curl.h
    HINTS ${CMAKE_PREFIX_PATH}
    PATH_SUFFIXES include)

find_library(CURL_LIBRARY NAMES curl
    HINTS ${CMAKE_PREFIX_PATH}
    PATH_SUFFIXES lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CURL DEFAULT_MSG
    CURL_LIBRARY CURL_INCLUDE_DIR)

if(CURL_FOUND)
    set(CURL_LIBRARIES ${CURL_LIBRARY})
    set(CURL_INCLUDE_DIRS ${CURL_INCLUDE_DIR})

    if(NOT TARGET CURL::libcurl)
        add_library(CURL::libcurl UNKNOWN IMPORTED)
        set_target_properties(CURL::libcurl PROPERTIES
            IMPORTED_LOCATION "${CURL_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${CURL_INCLUDE_DIR}")

        if(UNIX)
            set_property(TARGET CURL::libcurl APPEND PROPERTY INTERFACE_LINK_LIBRARIES "ssl;crypto;z;pthread")
        endif()
    endif()
endif()

mark_as_advanced(CURL_INCLUDE_DIR CURL_LIBRARY)
