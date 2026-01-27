# FindWAMR.cmake
# Finds the WAMR (WebAssembly Micro Runtime) library

find_path(WAMR_INCLUDE_DIR
    NAMES wasm_export.h
    PATHS ${CMAKE_PREFIX_PATH} ${CMAKE_INSTALL_PREFIX} /usr/local /usr
    PATH_SUFFIXES include
)

find_library(WAMR_LIBRARY
    NAMES iwasm vmlib
    PATHS ${CMAKE_PREFIX_PATH} ${CMAKE_INSTALL_PREFIX} /usr/local /usr
    PATH_SUFFIXES lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WAMR
    REQUIRED_VARS WAMR_LIBRARY WAMR_INCLUDE_DIR
)

if(WAMR_FOUND)
    set(WAMR_INCLUDE_DIRS ${WAMR_INCLUDE_DIR})
    set(WAMR_LIBRARIES ${WAMR_LIBRARY})
    if(NOT TARGET WAMR::WAMR)
        add_library(WAMR::WAMR UNKNOWN IMPORTED)
        set_target_properties(WAMR::WAMR PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${WAMR_INCLUDE_DIRS}"
            IMPORTED_LOCATION "${WAMR_LIBRARIES}"
        )
    endif()
endif()

mark_as_advanced(WAMR_INCLUDE_DIR WAMR_LIBRARY)
