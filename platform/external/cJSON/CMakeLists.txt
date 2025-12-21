# Simplified CMakeLists.txt for vendored cJSON

# Define the library target as a STATIC library
add_library(cjson STATIC
    cJSON.c
)

# Set the include directory.
# Since this is a PRIVATE dependency of the SDK, we don't need
# complex generator expressions for installation.
target_include_directories(cjson PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)