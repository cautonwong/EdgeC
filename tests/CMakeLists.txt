# Find the pre-compiled cmocka library
find_library(CMOCKA_LIB NAMES cmocka PATHS ${VENDOR_ROOT}/lib NO_DEFAULT_PATH)
find_path(CMOCKA_INCLUDE NAMES cmocka.h PATHS ${VENDOR_ROOT}/include NO_DEFAULT_PATH)

# Define the test executable
add_executable(unit_tests test_version.c)

# Link the core library and cmocka
target_include_directories(unit_tests PRIVATE ${CMOCKA_INCLUDE})
target_link_libraries(unit_tests PRIVATE edge_core_lib ${CMOCKA_LIB})

# Register with CTest
add_test(NAME VersionCheck COMMAND unit_tests)
