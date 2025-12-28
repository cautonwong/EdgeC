# - Find SQLite3
# This module finds an installed SQLite3 library.
#
# It sets the following variables:
#  SQLITE3_FOUND            - True if SQLite3 was found.
#  SQLITE3_INCLUDE_DIRS     - The SQLite3 include directories.
#  SQLITE3_LIBRARIES        - The SQLite3 libraries.

find_path(SQLITE3_INCLUDE_DIR sqlite3.h
    HINTS ${CMAKE_PREFIX_PATH}/include
    PATH_SUFFIXES include)

find_library(SQLITE3_LIBRARY NAMES sqlite3
    HINTS ${CMAKE_PREFIX_PATH}/lib
    PATH_SUFFIXES lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SQLite3 DEFAULT_MSG
    SQLITE3_LIBRARY SQLITE3_INCLUDE_DIR)

if(SQLITE3_FOUND)
    set(SQLITE3_LIBRARIES ${SQLITE3_LIBRARY})
    set(SQLITE3_INCLUDE_DIRS ${SQLITE3_INCLUDE_DIR})

    if(NOT TARGET SQLite::SQLite3)
        add_library(SQLite::SQLite3 UNKNOWN IMPORTED)
        set_target_properties(SQLite::SQLite3 PROPERTIES
            IMPORTED_LOCATION "${SQLITE3_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${SQLITE3_INCLUDE_DIR}")
    endif()
endif()

mark_as_advanced(SQLITE3_INCLUDE_DIR SQLITE3_LIBRARY)
