# FindmbedTLS.cmake

find_path(MBEDTLS_INCLUDE_DIR NAMES mbedtls/ssl.h PATHS ${LOCAL_ROOT}/include)
find_library(MBEDTLS_LIBRARY NAMES mbedtls PATHS ${LOCAL_ROOT}/lib)
find_library(MBEDX509_LIBRARY NAMES mbedx509 PATHS ${LOCAL_ROOT}/lib)
find_library(MBEDCRYPTO_LIBRARY NAMES mbedcrypto PATHS ${LOCAL_ROOT}/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(mbedTLS DEFAULT_MSG MBEDTLS_LIBRARY MBEDTLS_INCLUDE_DIR)

if(MBEDTLS_FOUND)
    add_library(mbedTLS::mbedtls UNKNOWN IMPORTED)
    set_target_properties(mbedTLS::mbedtls PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${MBEDTLS_INCLUDE_DIR}"
        IMPORTED_LOCATION "${MBEDTLS_LIBRARY}"
        INTERFACE_LINK_LIBRARIES "${MBEDX509_LIBRARY};${MBEDCRYPTO_LIBRARY}"
    )
endif()
