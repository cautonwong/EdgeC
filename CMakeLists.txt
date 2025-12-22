# C-EdgeX Project Root CMakeLists.txt

cmake_minimum_required(VERSION 3.22)
project(C-EdgeX C)

# Set global compile options
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wmissing-prototypes -Wstrict-prototypes")
add_compile_definitions(_GNU_SOURCE)

# Find required packages
find_package(Threads REQUIRED)

# Find the clang-tidy program (optional)
find_program(CLANG_TIDY clang-tidy)
if(CLANG_TIDY)
  message(STATUS "clang-tidy found: ${CLANG_TIDY}")
  set(CMAKE_C_CLANG_TIDY ${CLANG_TIDY}
    -extra-arg=-Wno-unknown-warning-option
    -extra-arg=-Wno-error=unknown-warning-option
    -p=${CMAKE_BINARY_DIR}
    -checks=-*,readability-*,modernize-*,clang-analyzer-*
  )
else()
  message(STATUS "clang-tidy not found. Skipping static analysis.")
endif()

# Add the platform (which defines the SDK) and the application
add_subdirectory(platform)
add_subdirectory(app)
