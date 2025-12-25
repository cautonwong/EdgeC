cmake_minimum_required(VERSION 3.15)
project(sqlite3 C)

# 1. 编译选项 (工业级配置)
add_compile_definitions(
    # 线程安全
    SQLITE_THREADSAFE=1
    # 启用列元数据 (许多 ORM 需要)
    SQLITE_ENABLE_COLUMN_METADATA
    # 启用 R-Tree 索引 (地理位置查询)
    SQLITE_ENABLE_RTREE
    # 启用 JSON 支持 (EdgeX/IoT 必备)
    SQLITE_ENABLE_JSON1
    # 启用 FTS5 全文搜索 (日志搜索用)
    SQLITE_ENABLE_FTS5
    # 声音报警 (防止某些误操作)
    SQLITE_ENABLE_UNLOCK_NOTIFY
)

# 2. 定义静态库
add_library(sqlite3_static STATIC sqlite3.c)
set_target_properties(sqlite3_static PROPERTIES OUTPUT_NAME "sqlite3")
# 这里的 -fPIC 很重要，确保静态库能被链接进动态库 (.so)
set_property(TARGET sqlite3_static PROPERTY POSITION_INDEPENDENT_CODE ON)

# 3. 定义命令行工具 (可选，用于调试)
add_executable(sqlite3_shell shell.c sqlite3.c)
# 链接系统库 (Linux 下通常需要 dl 和 pthread)
if(UNIX)
    target_link_libraries(sqlite3_shell pthread dl m)
endif()

# 4. 安装规则
include(GNUInstallDirs)
install(TARGETS sqlite3_static ARCHIVE DESTINATION lib)
install(FILES sqlite3.h sqlite3ext.h DESTINATION include)
install(TARGETS sqlite3_shell RUNTIME DESTINATION bin)
