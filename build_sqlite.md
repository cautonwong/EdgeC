这是一个非常具体且必要的工程化环节。

虽然 `sqlite` 官方 GitHub 仓库是一个镜像，且依赖 `Tcl` 脚本生成代码，直接编译源码非常麻烦。在嵌入式工业界，**标准做法**是使用 **SQLite Amalgamation**（合并版，即 `sqlite3.c` 和 `sqlite3.h`）。

为了融入我们的工程化体系，我们需要做两件事：
1.  **准备源码**：将 Amalgamation 文件放入 `vendor/sqlite` 并为其添加一个 `CMakeLists.txt`（使其适配我们的构建系统）。
2.  **编写构建脚本**：更新 `build_vendors.sh` 以支持多架构编译。

---

### 1. 准备 SQLite 源码与构建文件

请按照以下步骤操作，将 SQLite 变成一个标准的 CMake 工程。

#### 步骤 A: 下载并解压 Amalgamation
不要直接用 `git submodule` 拉取官方开发仓库（除非你想安装 Tcl 并手动生成代码）。

```bash
cd vendor
# 下载 SQLite 3.45.0 (示例)
wget https://www.sqlite.org/2024/sqlite-amalgamation-3450000.zip
unzip sqlite-amalgamation-3450000.zip
mv sqlite-amalgamation-3450000 sqlite
rm sqlite-amalgamation-3450000.zip
```

此时 `vendor/sqlite/` 下应该有 `sqlite3.c`, `sqlite3.h`, `shell.c` 等文件。

#### 步骤 B: 添加 `CMakeLists.txt`
在 `vendor/sqlite/` 目录下新建 `CMakeLists.txt`。这一步是将 SQLite 纳入我们自动化构建体系的关键。

**`vendor/sqlite/CMakeLists.txt`**

```cmake
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
```

---

### 2. 更新构建脚本 (`tools/build_vendors.sh`)

在之前的 `build_vendors.sh` 中增加 `build_sqlite` 函数。

```bash
# ... (之前的 build_cjson, build_libuv 函数保持不变)

# =========================================================
# 函数：编译 SQLite3
# =========================================================
build_sqlite() {
    ARCH=$1
    echo ">>> [SQLite3] Building for $ARCH..."
    
    SOURCE_DIR="$PROJECT_ROOT/vendor/sqlite"
    BUILD_DIR="$SOURCE_DIR/build-$ARCH"
    
    # 确定安装路径
    if [ "$ARCH" == "host" ]; then
        INST_DIR="$INSTALL_ROOT/linux-x86_64"
        OPTS=""
    else
        INST_DIR="$INSTALL_ROOT/linux-$ARCH"
        OPTS="-DCMAKE_TOOLCHAIN_FILE=$PROJECT_ROOT/docker/toolchains/aarch64-linux-gnu.cmake"
    fi

    # 清理并创建构建目录
    rm -rf "$BUILD_DIR" && mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # 执行 CMake
    cmake -DCMAKE_INSTALL_PREFIX="$INST_DIR" \
          -DCMAKE_BUILD_TYPE=Release \
          $OPTS ..
          
    # 编译并安装
    make -j$(nproc) && make install
    
    echo ">>> SQLite3 for [$ARCH] installed to $INST_DIR"
}

# ... (在主循环中调用 build_sqlite $TARGET)
```

---

### 3. 执行构建与验证

回到项目根目录，运行脚本：

```bash
# 如果在 Docker 容器内，或者本机已安装交叉编译链
./tools/build_vendors.sh
```

**预期结果**：
你会在 `.local` 目录下看到完整的 SQLite 产物：

```text
.local/
├── linux-x86_64/
│   ├── bin/
│   │   └── sqlite3_shell      # x86 调试工具
│   ├── include/
│   │   └── sqlite3.h
│   └── lib/
│       └── libsqlite3.a       # x86 静态库
└── linux-aarch64/
    ├── bin/
    │   └── sqlite3_shell      # ARM64 调试工具 (无法在 x86 运行)
    ├── include/
    │   └── sqlite3.h
    └── lib/
        └── libsqlite3.a       # ARM64 静态库
```

---

### 4. 主工程如何引用？

在你的主 `CMakeLists.txt` 中，因为我们已经设置了 `CMAKE_PREFIX_PATH` 指向 `.local`，所以查找 SQLite 非常简单：

```cmake
# 查找 SQLite3
find_library(SQLITE_LIB NAMES sqlite3 REQUIRED)
find_path(SQLITE_INC NAMES sqlite3.h REQUIRED)

# 链接到你的核心库
target_include_directories(edge_core PRIVATE ${SQLITE_INC})
target_link_libraries(edge_core PRIVATE ${SQLITE_LIB})

# 必须链接系统线程库和 dl 库 (SQLite 依赖)
target_link_libraries(edge_core PRIVATE pthread dl)
```

### 🎯 方案优势

1.  **纯净无依赖**：只需要 CMake 和 GCC，不需要安装 Tcl，不需要配置 Autotools。
2.  **功能全开**：通过 `CMakeLists.txt` 中的宏定义，我们显式开启了 JSON、R-Tree 等高级功能，这对于处理复杂 IoT 数据（如 EdgeX 风格的配置）至关重要。
3.  **调试方便**：顺手编译出的 `sqlite3_shell` 可以直接拷到开发板上 (`scp .local/linux-aarch64/bin/sqlite3_shell root@board:/usr/bin/`)，用于现场调试数据库，非常实用。