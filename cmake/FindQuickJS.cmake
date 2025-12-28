# - Find QuickJS
# This module finds an installed QuickJS library.
#
# It sets the following variables:
#  QUICKJS_FOUND            - True if QuickJS was found.
#  QUICKJS_INCLUDE_DIRS     - The QuickJS include directories.
#  QUICKJS_LIBRARIES        - The QuickJS libraries.

find_path(QUICKJS_INCLUDE_DIR quickjs.h
    HINTS ${CMAKE_PREFIX_PATH}
    PATH_SUFFIXES include include/quickjs)

find_library(QUICKJS_LIBRARY NAMES quickjs
    HINTS ${CMAKE_PREFIX_PATH}
    PATH_SUFFIXES lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(QuickJS DEFAULT_MSG
    QUICKJS_LIBRARY QUICKJS_INCLUDE_DIR)

if(QUICKJS_FOUND)
    set(QUICKJS_LIBRARIES ${QUICKJS_LIBRARY})
    set(QUICKJS_INCLUDE_DIRS ${QUICKJS_INCLUDE_DIR})

    if(NOT TARGET QuickJS::QuickJS)
        add_library(QuickJS::QuickJS UNKNOWN IMPORTED)
        set_target_properties(QuickJS::QuickJS PROPERTIES
            IMPORTED_LOCATION "${QUICKJS_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${QUICKJS_INCLUDE_DIR}")

        # QuickJS often needs libm and libdl
        if(UNIX)
            set_property(TARGET QuickJS::QuickJS APPEND PROPERTY INTERFACE_LINK_LIBRARIES "m;dl")
        endif()
    endif()
endif()

mark_as_advanced(QUICKJS_INCLUDE_DIR QUICKJS_LIBRARY)
