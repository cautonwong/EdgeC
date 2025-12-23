#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "cmocka::cmocka" for configuration "Release"
set_property(TARGET cmocka::cmocka APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(cmocka::cmocka PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libcmocka.so.1.0.0"
  IMPORTED_SONAME_RELEASE "libcmocka.so.0"
  )

list(APPEND _cmake_import_check_targets cmocka::cmocka )
list(APPEND _cmake_import_check_files_for_cmocka::cmocka "${_IMPORT_PREFIX}/lib/libcmocka.so.1.0.0" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
