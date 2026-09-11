set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)
set(CMAKE_FIND_ROOT_PATH /usr/aarch64-linux-gnu /usr ${CMAKE_CURRENT_SOURCE_DIR}/.local/linux-aarch64)
list(APPEND CMAKE_LIBRARY_PATH /usr/lib/aarch64-linux-gnu ${CMAKE_CURRENT_SOURCE_DIR}/.local/linux-aarch64/lib)
list(APPEND CMAKE_INCLUDE_PATH /usr/include/aarch64-linux-gnu /usr/include ${CMAKE_CURRENT_SOURCE_DIR}/.local/linux-aarch64/include)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)



