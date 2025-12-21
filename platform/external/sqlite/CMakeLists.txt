# platform/external/sqlite/CMakeLists.txt

add_library(sqlite3 STATIC
    sqlite3.c
)

# The shell tool needs to be built with -DSQLITE_ENABLE_COLUMN_METADATA
target_compile_definitions(sqlite3 PRIVATE SQLITE_ENABLE_COLUMN_METADATA)

target_include_directories(sqlite3 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
