# Zephyr Application CMakeLists.txt
cmake_minimum_required(VERSION 3.20)

# 1. 寻找 Zephyr 框架
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE} ${ZEPHYR_BASE})

# 2. 定义项目
project(edge_node_zephyr)

# 3. 添加源代码 (按最佳实践：逻辑放在 src/)
target_sources(app PRIVATE src/main.c)

# 4. (可选) 如果需要包含项目根目录的头文件
# target_include_directories(app PRIVATE "../../include")
