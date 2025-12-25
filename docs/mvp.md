这是一个**可以立即用于生产环境初始化的“种子工程” (Project Seed)**。

它虽然代码逻辑最简（只包含版本打印），但它拥有**完整的工业级骨架**：包含构建系统、依赖管理、代码规范、单元测试、内存检查、以及**基于 GitHub Actions 的自动化流水线**。

你可以直接在本地创建文件夹，把这些文件放进去，提交到 GitHub，你的 CI/CD 就会立即开始工作。

---

### 📂 1. 工程目录结构 (Directory Layout)

```text
edge-os-seed/
├── .github/
│   └── workflows/
│       └── ci.yml           # GitHub Actions 流水线定义
├── cmake/
│   └── Sanitizers.cmake     # ASan/UBSan 编译选项封装
├── docs/
│   └── adr/                 # 架构决策记录 (ADR)
│       └── 0001-init.md
├── external/                # 第三方库 (Git Submodule)
├── include/
│   └── edge_version.h       # 公共头文件
├── src/
│   ├── main.c               # 主程序入口
│   └── edge_version.c       # 核心逻辑实现
├── tests/
│   ├── CMakeLists.txt
│   └── test_version.c       # 单元测试
├── .clang-format            # 代码格式化规范
├── .gitignore               # Git 忽略规则
├── CMakeLists.txt           # 顶层构建脚本
└── README.md                # 项目说明
```

---

### 🛠️ 2. 构建系统 (Modern CMake)

#### `CMakeLists.txt` (顶层)
我们使用 Modern CMake，强制开启严格告警，并集成测试和安装目标。

```cmake
cmake_minimum_required(VERSION 3.15)
project(EdgeOS_Seed VERSION 0.1.0 LANGUAGES C)

# ==============================================================================
# 1. 全局配置 (Global Configuration)
# ==============================================================================
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # 生成 compile_commands.json 供 IDE 使用

# 引入自定义 CMake 模块
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Sanitizers)

# ==============================================================================
# 2. 编译选项 (Compiler Options)
# ==============================================================================
# 极其严格的告警，生产环境视为错误
add_compile_options(
    -Wall -Wextra -Wshadow -Wformat=2 -Wunused -Wconversion
    $<$<CONFIG:Release>:-Werror> 
)

# Debug 模式下开启 ASan (内存检测)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    enable_sanitizers(address undefined)
endif()

# ==============================================================================
# 3. 核心库目标 (Core Library Target)
# ==============================================================================
include_directories(include)

add_library(edge_core_lib STATIC
    src/edge_version.c
)

# ==============================================================================
# 4. 可执行文件目标 (Executable Target)
# ==============================================================================
add_executable(edge_app src/main.c)
target_link_libraries(edge_app PRIVATE edge_core_lib)

# ==============================================================================
# 5. 测试集成 (Testing)
# ==============================================================================
enable_testing()
add_subdirectory(tests)
```

#### `cmake/Sanitizers.cmake` (内存安全模块)
```cmake
function(enable_sanitizers project_name)
    message(STATUS "Enabling AddressSanitizer and UndefinedBehaviorSanitizer...")
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endfunction()
```

---

### 💻 3. 核心代码 (Minimal Implementation)

#### `include/edge_version.h`
```c
#ifndef EDGE_VERSION_H
#define EDGE_VERSION_H

const char* edge_get_version(void);
int edge_add(int a, int b); // 用于演示测试

#endif
```

#### `src/edge_version.c`
```c
#include "edge_version.h"

const char* edge_get_version(void) {
    return "EdgeOS v0.1.0 (Alpha)";
}

int edge_add(int a, int b) {
    return a + b;
}
```

#### `src/main.c`
```c
#include <stdio.h>
#include "edge_version.h"

int main(void) {
    printf("Starting %s...\n", edge_get_version());
    printf("Self-Test: 1+1=%d\n", edge_add(1, 1));
    return 0;
}
```

---

### 🧪 4. 单元测试 (CMocka Integration)

我们假设 CI 环境会安装 `libcmocka-dev`，或者你可以用 `FetchContent` 拉取。这里演示标准用法。

#### `tests/CMakeLists.txt`
```cmake
find_package(cmocka REQUIRED)

# 定义测试可执行文件
add_executable(unit_tests test_version.c)

# 链接核心库和 cmocka
target_link_libraries(unit_tests PRIVATE edge_core_lib cmocka)

# 注册到 CTest
add_test(NAME VersionCheck COMMAND unit_tests)
```

#### `tests/test_version.c`
```c
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "edge_version.h"

static void test_addition(void **state) {
    (void) state; /* unused */
    assert_int_equal(edge_add(2, 3), 5);
    assert_int_equal(edge_add(-1, 1), 0);
}

static void test_version_string(void **state) {
    (void) state;
    const char *v = edge_get_version();
    assert_non_null(v);
    assert_string_equal(v, "EdgeOS v0.1.0 (Alpha)");
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_addition),
        cmocka_unit_test(test_version_string),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
```

---

### ⚙️ 5. 代码规范 (.clang-format)

基于 Google 风格但针对嵌入式调整（允许每行 100 字符）。

```yaml
Language: Cpp
BasedOnStyle: Google
ColumnLimit: 100
IndentWidth: 4
TabWidth: 4
UseTab: Never
BreakBeforeBraces: Attach
AllowShortFunctionsOnASingleLine: None
```

---

### 🚀 6. CI/CD 流水线 (GitHub Actions)

这是工程化的核心。我们定义一个 Workflow，包含 **构建、测试、内存检查、静态分析**。

#### `.github/workflows/ci.yml`

```yaml
name: EdgeOS CI Pipeline

on:
  push:
    branches: [ "main", "develop" ]
  pull_request:
    branches: [ "main" ]

jobs:
  build-and-test:
    name: Build & Test (Linux)
    runs-on: ubuntu-22.04
    
    steps:
      - name: Checkout Code
        uses: actions/checkout@v4
        with:
          submodules: recursive

      - name: Install Dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y cmake gcc libcmocka-dev cppcheck valgrind

      - name: Static Analysis (Cppcheck)
        run: |
          # 检查 src 目录，如果有错误直接 fail
          cppcheck --enable=all --error-exitcode=1 --suppress=missingIncludeSystem src/

      - name: Check Code Formatting
        run: |
          # 检查代码格式是否符合 .clang-format
          # 如果不想在 CI 中强制失败，可以去掉这一步
          clang-format -n --Werror src/*.c include/*.h

      - name: Configure CMake (Debug + ASan)
        run: |
          mkdir build
          cd build
          cmake -DCMAKE_BUILD_TYPE=Debug ..

      - name: Compile
        run: |
          cd build
          make -j$(nproc)

      - name: Run Unit Tests (with ASan)
        run: |
          cd build
          # CTest 会自动运行注册的测试用例
          # 因为开启了 ASan，如果内存越界，测试会直接 Crash 导致 CI 失败
          ctest --output-on-failure

      - name: Run Memory Leak Check (Valgrind)
        # 注意：ASan 和 Valgrind 冲突，这里通常建议分开 Job 跑
        # 演示目的，我们重新编译一个不带 ASan 的版本跑 Valgrind
        run: |
          rm -rf build_rel
          mkdir build_rel && cd build_rel
          cmake -DCMAKE_BUILD_TYPE=Release ..
          make
          valgrind --leak-check=full --error-exitcode=1 ./tests/unit_tests
```

---

### 📝 7. 架构决策记录 (ADR)

创建 `docs/adr/0001-init-project.md`，这代表了工程的正规性。

```markdown
# 1. Project Initialization

## Status
Accepted

## Context
We need a high-reliability C environment for EdgeOS development (15+ years lifecycle).

## Decision
1. Use **CMake** as the build system for portability.
2. Use **GitHub Actions** for CI/CD.
3. Enforce **C11** standard.
4. Use **CMocka** for unit testing due to its lightweight nature and no C++ dependency.
5. Enable **AddressSanitizer** in Debug builds by default.

## Consequences
- Developers must install CMake 3.15+.
- All PRs must pass static analysis and ASan checks.
```

---

### 如何开始？

1.  在本地创建一个文件夹，把上面这些文件内容复制进去。
2.  初始化 git：
    ```bash
    git init
    git add .
    git commit -m "Initial commit: EdgeOS Seed"
    ```
3.  在 GitHub 上新建一个仓库。
4.  推送到 GitHub：
    ```bash
    git remote add origin https://github.com/yourname/edge-os.git
    git push -u origin main
    ```
5.  **见证奇迹**：点击 GitHub 仓库的 "Actions" 标签页，你会看到一条流水线正在运行。如果它变绿了，说明你的**工业级最小系统**已经就绪。

这是一个**非常关键的补充**。

为了保证 15 年后你的代码还能被编译，**必须把编译环境（OS + 编译器 + 依赖库）“冻结”在一个 Docker 镜像里**。

这就是所谓的 **“环境即代码” (Infrastructure as Code)**。

以下是专为 EdgeOS 项目设计的 Docker 构建环境方案，包含**Dockerfile**、**交叉编译工具链**以及**便捷启动脚本**。

---

### 📂 1. 新增目录结构

将这些文件添加到工程根目录：

```text
edge-os-seed/
├── docker/
│   ├── Dockerfile           # 构建镜像的配方
│   ├── entrypoint.sh        # 容器启动脚本
│   └── toolchains/
│       └── aarch64-linux-gnu.cmake # 交叉编译工具链文件
├── docker_build.sh          # 开发者一键编译脚本
```

---

### 🐳 2. 核心 Dockerfile (支持 x86 和 ARM 交叉编译)

我们选择 **Debian Bookworm (Stable)** 作为基底，因为它极其稳定，且包含较新的 GCC/CMake。

**文件：`docker/Dockerfile`**

```dockerfile
# 使用 Debian 12 (Bookworm) Slim 版本，体积小且稳定
FROM debian:bookworm-slim

# 设置元数据
LABEL maintainer="EdgeOS Team"
LABEL description="Build environment for EdgeOS (x86_64 & aarch64)"

# 避免 apt 安装时的交互提示
ENV DEBIAN_FRONTEND=noninteractive

# 1. 安装基础构建工具和依赖库
# build-essential: GCC, Make 等
# cmake, ninja-build: 构建系统
# git: 版本控制
# libuv1-dev, libsqlite3-dev: 核心运行时依赖
# libcmocka-dev: 单元测试依赖
# cppcheck, valgrind, clang-format: 质量工具
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    git \
    pkg-config \
    libuv1-dev \
    libsqlite3-dev \
    libcmocka-dev \
    libssl-dev \
    cppcheck \
    valgrind \
    clang-format \
    lcov \
    && rm -rf /var/lib/apt/lists/*

# 2. 安装交叉编译工具链 (针对 ARM64 边缘设备，如 RK3568, 树莓派)
RUN apt-get update && apt-get install -y --no-install-recommends \
    gcc-aarch64-linux-gnu \
    g++-aarch64-linux-gnu \
    libc6-dev-arm64-cross \
    && rm -rf /var/lib/apt/lists/*

# 3. 创建构建用户 (避免生成的产物是 root 权限)
ARG USER_ID=1000
ARG GROUP_ID=1000
RUN groupadd -g ${GROUP_ID} builder && \
    useradd -m -u ${USER_ID} -g builder -s /bin/bash builder

# 4. 配置工作目录
USER builder
WORKDIR /home/builder/src

# 5. 复制工具链文件 (将在下一步创建)
COPY --chown=builder:builder toolchains/ /home/builder/toolchains/

# 默认指令
CMD ["/bin/bash"]
```

---

### 🔗 3. 交叉编译工具链文件

CMake 需要这个文件来知道“我要用 ARM 的编译器，而不是 x86 的”。

**文件：`docker/toolchains/aarch64-linux-gnu.cmake`**

```cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# 指定编译器
set(CMAKE_C_COMPILER   /usr/bin/aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/aarch64-linux-gnu-g++)

# 指定查找路径 (Sysroot)
set(CMAKE_FIND_ROOT_PATH  /usr/aarch64-linux-gnu)

# 调整查找行为：
# 程序(make, ninja) 在 Host 找
# 库和头文件 在 Target (Sysroot) 找
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
```

---

### 🐚 4. 开发者一键脚本 (`docker_build.sh`)

这是给团队成员用的“傻瓜式”脚本。不论他们在 Windows (WSL)、Mac 还是 Linux 上，执行这个脚本就能得到一样的结果。

**文件：`docker_build.sh`**

```bash
#!/bin/bash
set -e

# 镜像名称
IMAGE_NAME="edgeos-builder:v1.0"
# 项目根目录
PROJECT_ROOT=$(pwd)

# 1. 检查是否需要构建镜像
if [[ "$(docker images -q $IMAGE_NAME 2> /dev/null)" == "" ]]; then
    echo "[EdgeOS] Building Docker image..."
    # 传入当前用户的 UID，防止生成的文件属于 root
    docker build \
        --build-arg USER_ID=$(id -u) \
        --build-arg GROUP_ID=$(id -g) \
        -t $IMAGE_NAME -f docker/Dockerfile docker/
fi

# 2. 定义编译类型 (默认本机 x86，传入 "arm" 则交叉编译)
BUILD_TYPE=${1:-x86}
BUILD_DIR="build_docker_${BUILD_TYPE}"

echo "[EdgeOS] Compiling for target: $BUILD_TYPE ..."

# 3. 运行容器进行编译
# -v: 挂载当前代码目录到容器内
# --rm: 跑完即删容器
docker run --rm -it \
    -v "$PROJECT_ROOT":/home/builder/src \
    $IMAGE_NAME /bin/bash -c "
        mkdir -p $BUILD_DIR && cd $BUILD_DIR
        
        if [ '$BUILD_TYPE' == 'arm' ]; then
            # 交叉编译配置
            cmake -DCMAKE_TOOLCHAIN_FILE=/home/builder/toolchains/aarch64-linux-gnu.cmake \
                  -DCMAKE_BUILD_TYPE=Release ..
        else
            # 本机编译配置 (带 ASan)
            cmake -DCMAKE_BUILD_TYPE=Debug ..
        fi
        
        # 编译
        make -j\$(nproc)
        
        # 如果是本机编译，运行测试
        if [ '$BUILD_TYPE' == 'x86' ]; then
            echo '[EdgeOS] Running Tests...'
            ctest --output-on-failure
        fi
    "

echo "[EdgeOS] Build finished. Artifacts are in $BUILD_DIR"
```

给脚本加执行权限：
```bash
chmod +x docker_build.sh
```

---

### 🚀 5. 使用指南

#### 场景 A：日常开发与测试 (x86_64)
开发者只需运行：
```bash
./docker_build.sh
```
**发生了什么？**
1.  自动构建 Docker 镜像。
2.  启动容器，挂载源码。
3.  执行 CMake (Debug 模式 + ASan)。
4.  编译代码。
5.  **自动运行 `ctest` 单元测试**。
6.  退出容器，在 `build_docker_x86/` 下留下编译好的程序。

#### 场景 B：生成产品固件 (ARM64 / RK3568)
开发者运行：
```bash
./docker_build.sh arm
```
**发生了什么？**
1.  启动容器。
2.  加载 `aarch64-linux-gnu.cmake` 工具链。
3.  执行 CMake (Release 模式)。
4.  使用 `aarch64-linux-gnu-gcc` 编译。
5.  跳过单元测试（因为 x86 容器无法运行 ARM 二进制）。
6.  在 `build_docker_arm/` 下生成可在板子上运行的 `edge_app`。

---

### ⚙️ 6. CI 流水线集成 (GitHub Actions)

修改之前的 `.github/workflows/ci.yml`，直接使用这个 Docker 镜像，确保 **CI 环境与开发环境 100% 一致**。

```yaml
name: EdgeOS CI (Dockerized)

on: [push, pull_request]

jobs:
  build-in-docker:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive

      # 1. 构建镜像 (利用 Docker Layer Caching)
      - name: Build Docker Image
        run: docker build -t edgeos-builder:latest -f docker/Dockerfile docker/

      # 2. 在容器内运行代码检查
      - name: Static Analysis
        run: |
          docker run --rm -v $(pwd):/home/builder/src edgeos-builder:latest \
          cppcheck --enable=all --error-exitcode=1 src/

      # 3. 在容器内编译并测试 (x86)
      - name: Build & Test (x86)
        run: |
          docker run --rm -v $(pwd):/home/builder/src edgeos-builder:latest \
          /bin/bash -c "mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Debug .. && make && ctest --output-on-failure"

      # 4. 在容器内交叉编译 (ARM64)
      - name: Cross Compile (ARM64)
        run: |
          docker run --rm -v $(pwd):/home/builder/src edgeos-builder:latest \
          /bin/bash -c "mkdir build_arm && cd build_arm && cmake -DCMAKE_TOOLCHAIN_FILE=/home/builder/toolchains/aarch64-linux-gnu.cmake .. && make"
```

### 总结

加上这个 `docker/` 目录，你的工程化水平就达到了 **Tier 1** 级别：

1.  **一致性**：新员工入职，装个 Docker，运行脚本，5分钟内就能编译通过。不用配置 GCC 版本、不用装库。
2.  **长期维护**：15 年后，只要你有这个 Dockerfile，你就能还原出今天的编译环境。
3.  **跨平台**：轻松搞定 ARM 交叉编译，不用在主机上折腾复杂的工具链路径。

这是一个非常典型的**“密封式构建”（Hermetic Build）**策略，非常适合长生命周期的工业项目。

这样做的好处是：
1.  **环境隔离**：15 年后即使 GitHub 倒闭、依赖库源码消失，你的 Git 仓库里依然有能用的二进制库。
2.  **构建极速**：日常开发不需要编译繁重的第三方库（如 OpenSSL, libuv）。
3.  **多平台一致性**：所有开发者和 CI 使用完全相同的二进制库，消除了“我本地能跑”的问题。

以下是完整的工程化落地代码和脚本。

---

### 📂 1. 目录结构设计 (Layout)

我们设计一个清晰的结构，将**源码** (`vendor`) 和 **产物** (`.local`) 分离。

```text
edge-os-project/
├── CMakeLists.txt           # 主项目构建脚本
├── build_vendors.sh         # [核心] 第三方库编译脚本 (手动运行)
├── vendor/                  # 第三方库源码 (Git Submodules)
│   ├── cJSON/
│   └── libuv/
├── .local/                  # [核心] 预编译产物 (提交到 Git)
│   ├── linux-x86_64/        # 开发机环境
│   │   ├── include/
│   │   └── lib/
│   └── linux-aarch64/       # 边缘设备环境
│       ├── include/
│       └── lib/
└── .gitignore               # 配置 Git 规则
```

---

### 📜 2. 配置 .gitignore (关键)

这一步至关重要。通常我们忽略二进制文件，但这次我们要**反其道而行之**，强制提交 `.local`，同时忽略 vendor 下的临时构建文件。

**`.gitignore`**
```gitignore
# 1. 忽略所有编译目录
build/
build_*/

# 2. 忽略 vendor 目录下的临时构建文件
vendor/*/build/
vendor/*/out/

# 3. [关键] 允许提交 .local 目录下的库文件
!/.local/
!*.a
!*.so
!*.h
!*.cmake
```

*(注：如果生成的静态库 `.a` 文件非常大，建议后续配置 Git LFS，但对于 cJSON/libuv 这种小库直接提交即可)*

---

### 🛠️ 3. 编写 `build_vendors.sh` (核心脚本)

这个脚本是整个方案的引擎。它负责：
1.  识别目标架构（宿主机或交叉编译）。
2.  进入 `vendor` 目录编译。
3.  将结果安装到 `.local/<arch>/`。

**`build_vendors.sh`**

```bash
#!/bin/bash
set -e # 遇到错误立即退出

# 项目根目录
PROJECT_ROOT=$(pwd)
INSTALL_ROOT="$PROJECT_ROOT/.local"

# 定义要构建的架构列表
# 如果你需要更多架构，在这里添加
TARGETS=("host" "arm64")

# =========================================================
# 函数：编译 cJSON
# =========================================================
build_cjson() {
    ARCH=$1
    echo ">>> Building cJSON for [$ARCH] ..."
    
    SOURCE_DIR="$PROJECT_ROOT/vendor/cJSON"
    BUILD_DIR="$SOURCE_DIR/build-$ARCH"
    INSTALL_DIR="$INSTALL_ROOT/linux-$ARCH"
    
    # 清理旧构建
    rm -rf "$BUILD_DIR" && mkdir -p "$BUILD_DIR"
    
    cd "$BUILD_DIR"
    
    # 基础 CMake 参数
    CMAKE_ARGS="-DCMAKE_INSTALL_PREFIX=$INSTALL_DIR -DENABLE_CJSON_TEST=OFF -DBUILD_SHARED_AND_STATIC_LIBS=ON -DCMAKE_BUILD_TYPE=Release"
    
    # 针对架构的特殊处理
    if [ "$ARCH" == "aarch64" ]; then
        # 交叉编译参数 (复用 docker 里的工具链文件)
        CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=$PROJECT_ROOT/docker/toolchains/aarch64-linux-gnu.cmake"
    elif [ "$ARCH" == "x86_64" ] || [ "$ARCH" == "host" ]; then
        # 宿主机编译，归一化名称
        INSTALL_DIR="$INSTALL_ROOT/linux-x86_64"
        # 重新生成 Makefile
        cmake $CMAKE_ARGS ..
    fi
    
    # 如果是 ARM，再次配置确保 Toolchain 生效 (对于某些库需要分步)
    if [ "$ARCH" == "aarch64" ]; then
        cmake $CMAKE_ARGS ..
    fi

    # 编译并安装
    make -j$(nproc)
    make install
    
    echo ">>> cJSON for [$ARCH] installed to $INSTALL_DIR"
}

# =========================================================
# 函数：编译 libuv (Autotools 示例，如果是 CMake 则类似 cJSON)
# =========================================================
build_libuv() {
    ARCH=$1
    echo ">>> Building libuv for [$ARCH] ..."
    
    SOURCE_DIR="$PROJECT_ROOT/vendor/libuv"
    # libuv 推荐 out-of-tree build 但 autotools 比较麻烦，这里演示 CMake 方式
    BUILD_DIR="$SOURCE_DIR/build-$ARCH"
    INSTALL_DIR="$INSTALL_ROOT/linux-$ARCH"
    
    # Host 修正
    if [ "$ARCH" == "host" ]; then INSTALL_DIR="$INSTALL_ROOT/linux-x86_64"; fi

    rm -rf "$BUILD_DIR" && mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    CMAKE_ARGS="-DCMAKE_INSTALL_PREFIX=$INSTALL_DIR -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release"
    
    if [ "$ARCH" == "aarch64" ]; then
        CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=$PROJECT_ROOT/docker/toolchains/aarch64-linux-gnu.cmake"
    fi
    
    cmake $CMAKE_ARGS ..
    make -j$(nproc)
    make install
    echo ">>> libuv for [$ARCH] installed."
}

# =========================================================
# 主循环
# =========================================================

# 1. 检测 host 架构
HOST_ARCH=$(uname -m)

for TARGET in "${TARGETS[@]}"; do
    # 映射 target 名称
    if [ "$TARGET" == "host" ]; then
        REAL_ARCH=$HOST_ARCH
    else
        REAL_ARCH=$TARGET
    fi
    
    echo "=========================================="
    echo " Building vendors for Target: $REAL_ARCH"
    echo "=========================================="
    
    # 在这里添加所有需要的库
    build_cjson $TARGET
    build_libuv $TARGET
    
    # 清理临时 build 目录 (减少 git status 干扰，虽然已被忽略)
    # rm -rf "$PROJECT_ROOT/vendor/*/build-$TARGET"
done

echo "✅ All vendors built successfully!"
```

赋予执行权限：
```bash
chmod +x build_vendors.sh
```

---

### ⚙️ 4. 修改主 `CMakeLists.txt` (消费预编译库)

主项目不再编译源码，而是根据当前架构去 `.local` 里找头文件和库文件。

**`CMakeLists.txt`**

```cmake
cmake_minimum_required(VERSION 3.15)
project(EdgeOS_App VERSION 1.0.0 LANGUAGES C)

# =========================================================
# 1. 确定当前平台目录名 (Host vs Cross)
# =========================================================
if(CMAKE_CROSSCOMPILING)
    # 如果是交叉编译，CMAKE_SYSTEM_PROCESSOR 通常由 Toolchain 文件指定
    set(TARGET_ARCH "linux-${CMAKE_SYSTEM_PROCESSOR}") 
else()
    # 如果是本机编译
    execute_process(COMMAND uname -m OUTPUT_VARIABLE HOST_ARCH OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(TARGET_ARCH "linux-${HOST_ARCH}")
endif()

message(STATUS ">>> Using Vendor Libs from: .local/${TARGET_ARCH}")

# =========================================================
# 2. 设置查找路径 (让 find_library / find_path 生效)
# =========================================================
set(VENDOR_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.local/${TARGET_ARCH}")

# 告诉 CMake 优先在 .local 下查找包
set(CMAKE_PREFIX_PATH ${VENDOR_ROOT} ${CMAKE_PREFIX_PATH})

# 也可以手动指定 include 目录
include_directories(${VENDOR_ROOT}/include)
link_directories(${VENDOR_ROOT}/lib)

# =========================================================
# 3. 查找并链接库 (Standard Way)
# =========================================================

# 查找 cJSON
find_library(CJSON_LIB NAMES cjson PATHS ${VENDOR_ROOT}/lib NO_DEFAULT_PATH)
find_path(CJSON_INCLUDE NAMES cJSON.h PATHS ${VENDOR_ROOT}/include/cjson NO_DEFAULT_PATH)

# 查找 libuv
find_library(LIBUV_LIB NAMES uv uv_a PATHS ${VENDOR_ROOT}/lib NO_DEFAULT_PATH)
# libuv 可能叫 libuv.a 或 libuv_a.a

# =========================================================
# 4. 构建主程序
# =========================================================
add_executable(edge_app src/main.c)

target_include_directories(edge_app PRIVATE ${CJSON_INCLUDE})
target_link_libraries(edge_app PRIVATE ${CJSON_LIB} ${LIBUV_LIB})

# 如果是静态链接，可能还需要系统库
if(UNIX)
    target_link_libraries(edge_app PRIVATE pthread dl m)
endif()
```

---

### 🚀 5. 工作流演示

#### 步骤 A: 首次初始化 (维护者操作)
这步只需要做一次，或者当第三方库版本更新时做。

```bash
# 1. 拉取 vendor 源码
git submodule add https://github.com/DaveGamble/cJSON.git vendor/cJSON
git submodule add https://github.com/libuv/libuv.git vendor/libuv

# 2. 编译所有架构的库 (耗时操作)
# 这一步会生成 .local/linux-x86_64 和 .local/linux-aarch64
./build_vendors.sh 

# 3. 提交产物到 Git
git add .local/
git commit -m "chore: update vendor libs (cJSON 1.7.15, libuv 1.44)"
git push
```

#### 步骤 B: 日常开发 (开发者操作)
开发者 clone 项目后，**不需要**编译 cJSON 和 libuv，直接编译业务代码。

```bash
git clone ...
# 注意：开发者甚至不需要 git submodule update，因为 .local 里已经有库了！
# 除非开发者需要修改第三方库源码调试。

# 正常编译业务代码
mkdir build && cd build
cmake ..
make
```

#### 步骤 C: 交叉编译 (Docker / CI)
```bash
# 在 Docker 容器内
cmake -DCMAKE_TOOLCHAIN_FILE=.../aarch64-linux-gnu.cmake ..
# CMake 会自动识别 CMAKE_SYSTEM_PROCESSOR 为 aarch64
# 并自动指向 .local/linux-aarch64/lib
make
```

---

### ✅ 方案总结

这个方案完美解决了你的需求：

1.  **Vendor 隔离**：源码在 `vendor/`，互不干扰。
2.  **一次编译，到处运行**：`.local` 目录里躺着所有架构的静态库。
3.  **多架构支持**：通过 `linux-x86_64` 和 `linux-aarch64` 物理隔离，杜绝了“链接了错误架构库”的低级错误。
4.  **极速构建**：主项目 CMake 运行飞快，因为它不需要递归编译第三方源码。
5.  **长期维护性**：10 年后，哪怕 cJSON 官网没了，你仓库里的 `.local/linux-x86_64/lib/libcjson.a` 依然能用。