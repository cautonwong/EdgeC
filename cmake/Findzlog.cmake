# Findzlog.cmake

find_path(ZLOG_INCLUDE_DIR NAMES zlog.h PATHS ${LOCAL_ROOT}/include)
find_library(ZLOG_LIBRARY NAMES zlog PATHS ${LOCAL_ROOT}/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(zlog DEFAULT_MSG ZLOG_LIBRARY ZLOG_INCLUDE_DIR)

if(ZLOG_FOUND)
    add_library(zlog::zlog UNKNOWN IMPORTED)
    set_target_properties(zlog::zlog PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${ZLOG_INCLUDE_DIR}"
        IMPORTED_LOCATION "${ZLOG_LIBRARY}"
    )
endif()
