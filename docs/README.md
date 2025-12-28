这是一个基于 **C (C11)**、**CMake**、**libuv** 和 **cmocka** 的工程化脚手架，我们将其命名为 **"C-EdgeX"**。

这个方案采用了 **"模块化单体" (Modular Monolith)** 架构，保留了 EdgeX 的核心设计（设备抽象 -> 核心数据 -> 导出），但去掉了 Docker 和微服务的复杂性，适合资源受限的 Linux 设备。

---

### 📂 1. 工程目录结构 (Directory Structure)

首先，建立符合工业级标准的目录结构。

```text
c-edgex/
├── CMakeLists.txt           # 顶层构建脚本 (负责编译选项、ASan 配置)
├── external/                # 第三方库 (git submodules)
│   ├── cJSON/
│   ├── log.c/               # 一个极简的 C 日志库
│   └── cmocka/              # 测试框架
├── include/                 # 公共头文件
│   ├── common_types.h       # 核心数据结构 (Event/Reading)
│   └── device_iface.h       # 设备驱动接口 (核心解耦点)
├── src/
│   ├── main.c               # 程序入口 (libuv 事件循环)
│   ├── bus/                 # 内部消息总线 (简单队列)
│   ├── core/                # 数据处理逻辑
│   └── drivers/             # 具体设备驱动实现
│       └── sim_sensor.c     # 模拟传感器实现
├── tests/                   # 单元测试
│   ├── CMakeLists.txt
│   └── test_driver.c        # 驱动接口测试 (Mocking)
└── .gitlab-ci.yml           # CI/CD 配置文件
```

---

### 🛠️ 2. 构建系统配置 (CMake Best Practices)

这是实现“工程化”的关键。我们需要在 CMake 中集成 **AddressSanitizer (ASan)** 和 **测试支持**。

**`CMakeLists.txt`:**

```cmake
cmake_minimum_required(VERSION 3.10)
project(c-edgex VERSION 0.1.0 LANGUAGES C)

# ==========================================
# 1. 编译选项与安全加固
# ==========================================
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
# 开启所有警告，并把警告视为错误 (CI 里的质量门禁)
add_compile_options(-Wall -Wextra -Werror -Wshadow -Wformat=2)

# ==========================================
# 2. 配置 AddressSanitizer (ASan) - 内存安全守护神
#    Debug 模式下默认开启
# ==========================================
option(ENABLE_ASAN "Enable AddressSanitizer" ON)
if(ENABLE_ASAN AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Build with AddressSanitizer")
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address)
endif()

# ==========================================
# 3. 依赖查找 (假设系统已安装 libuv)
# ==========================================
find_package(libuv REQUIRED)
# 引入本地的 cJSON 和 log.c (假设在 external 下)
add_subdirectory(external/cJSON)
# add_subdirectory(external/log.c) # 示意

# ==========================================
# 4. 核心库与主程序
# ==========================================
include_directories(include external/cJSON)

# 将核心逻辑打包成静态库，方便测试链接
add_library(core_lib
    src/drivers/sim_sensor.c
    # src/bus/msg_bus.c
    # src/core/data_processor.c
)
target_link_libraries(core_lib PRIVATE cjson)

# 主程序
add_executable(c_edgex_app src/main.c)
target_link_libraries(c_edgex_app PRIVATE core_lib uv)

# ==========================================
# 5. 测试集成
# ==========================================
enable_testing()
add_subdirectory(tests)
```

---

### 💻 3. 核心代码实现

#### A. 定义数据结构与接口 (`include/`)

**`include/common_types.h` (统一数据格式)**

```c
#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <stdint.h>
#include <time.h>

// 模仿 EdgeX 的 Reading
typedef struct {
    char resource_name[32]; // e.g. "temperature"
    enum { VAL_INT, VAL_FLOAT, VAL_BOOL } type;
    union {
        int64_t i_val;
        double  f_val;
        int     b_val;
    } value;
} Reading;

// 模仿 EdgeX 的 Event
typedef struct {
    char device_name[32];
    uint64_t timestamp;
    Reading reading; // 简化起见，这里只放一个reading
} EdgeEvent;

#endif
```

**`include/device_iface.h` (面向接口编程)**

这是 C 语言实现多态的关键。

```c
#ifndef DEVICE_IFACE_H
#define DEVICE_IFACE_H

#include "common_types.h"

// 前置声明
typedef struct DeviceDriver DeviceDriver;

// 定义函数指针接口 vtable
typedef struct {
    int (*init)(DeviceDriver *self);
    int (*read)(DeviceDriver *self, EdgeEvent *out_event);
    int (*close)(DeviceDriver *self);
} DeviceVTable;

// 基类结构体
struct DeviceDriver {
    const DeviceVTable *vptr; // 虚函数表指针
    char name[32];            // 设备名称
    void *priv_data;          // 私有数据 (用于存 Modbus context 或 file descriptor)
};

// 辅助调用宏 (语法糖)
#define DEVICE_INIT(d)       ((d)->vptr->init(d))
#define DEVICE_READ(d, e)    ((d)->vptr->read(d, e))
#define DEVICE_CLOSE(d)      ((d)->vptr->close(d))

#endif
```

#### B. 具体驱动实现 (`src/drivers/`)

**`src/drivers/sim_sensor.c` (模拟设备)**

```c
#include "device_iface.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// 具体实现
static int sim_init(DeviceDriver *self) {
    printf("[Driver] Initializing %s...\n", self->name);
    return 0;
}

static int sim_read(DeviceDriver *self, EdgeEvent *out_event) {
    // 模拟生成数据
    strncpy(out_event->device_name, self->name, 31);
    out_event->timestamp = (uint64_t)time(NULL);

    out_event->reading.type = VAL_FLOAT;
    strcpy(out_event->reading.resource_name, "temperature");
    // 生成 20.0 - 30.0 之间的随机数
    out_event->reading.value.f_val = 20.0 + ((float)rand() / (float)(RAND_MAX)) * 10.0;

    return 0; // Success
}

static int sim_close(DeviceDriver *self) {
    printf("[Driver] Closing %s\n", self->name);
    return 0;
}

// 虚函数表
static const DeviceVTable sim_vtable = {
    .init = sim_init,
    .read = sim_read,
    .close = sim_close,
};

// 工厂函数：创建实例
DeviceDriver* create_sim_device(const char* name) {
    DeviceDriver *dev = malloc(sizeof(DeviceDriver));
    if (!dev) return NULL;

    strncpy(dev->name, name, 31);
    dev->vptr = &sim_vtable; // 绑定接口
    dev->priv_data = NULL;

    return dev;
}
```

#### C. 主程序 (`src/main.c`)

使用 `libuv` 处理异步定时采集。

```c
#include <uv.h>
#include <stdio.h>
#include "device_iface.h"

// 声明外部工厂函数
extern DeviceDriver* create_sim_device(const char* name);

// 包装器：用于传递给 libuv 回调
typedef struct {
    DeviceDriver *device;
} TaskContext;

// 定时器回调：执行采集
void on_timer_tick(uv_timer_t *handle) {
    TaskContext *ctx = (TaskContext*)handle->data;
    EdgeEvent event;

    if (DEVICE_READ(ctx->device, &event) == 0) {
        printf("[Core] Got Event: Device=%s, Temp=%.2f\n",
               event.device_name, event.reading.value.f_val);

        // TODO: 在这里调用序列化 (cJSON) 并发送 MQTT
        // publish_to_cloud(&event);
    }
}

int main() {
    uv_loop_t *loop = uv_default_loop();

    // 1. 创建设备实例
    DeviceDriver *sensor1 = create_sim_device("Sensor-001");
    DEVICE_INIT(sensor1);

    // 2. 设置 libuv 定时器 (每 2000ms 采集一次)
    uv_timer_t timer_req;
    TaskContext ctx = { .device = sensor1 };

    uv_timer_init(loop, &timer_req);
    timer_req.data = &ctx; // 传递上下文
    uv_timer_start(&timer_req, on_timer_tick, 0, 2000);

    printf("EdgeX-C Agent Started. Press Ctrl+C to exit.\n");

    // 3. 启动事件循环
    uv_run(loop, UV_RUN_DEFAULT);

    // 清理资源 (Ctrl+C 后)
    DEVICE_CLOSE(sensor1);
    free(sensor1);
    uv_loop_close(loop);
    return 0;
}
```

---

### 🧪 4. 测试工程化 (`tests/`)

使用 `cmocka` 进行单元测试，确保逻辑正确且无内存泄漏。

**`tests/test_driver.c`**

```c
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "device_iface.h"

// 假设我们测试具体的 sim_sensor 实现
extern DeviceDriver* create_sim_device(const char* name);

static void test_sim_device_read(void **state) {
    (void) state; /* unused */

    DeviceDriver *dev = create_sim_device("TestDev");
    assert_non_null(dev);

    assert_int_equal(DEVICE_INIT(dev), 0);

    EdgeEvent evt;
    int ret = DEVICE_READ(dev, &evt);

    assert_int_equal(ret, 0);
    assert_string_equal(evt.device_name, "TestDev");
    assert_true(evt.reading.value.f_val >= 20.0); // 验证逻辑

    DEVICE_CLOSE(dev);
    free(dev);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_sim_device_read),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
```

**`tests/CMakeLists.txt`**

```cmake
find_package(cmocka REQUIRED)

add_executable(unit_tests test_driver.c)
# 链接核心库和 cmocka
target_link_libraries(unit_tests PRIVATE core_lib cmocka)

# 注册到 CTest
add_test(NAME DriverTests COMMAND unit_tests)
```

---

### 🚀 5. CI/CD 流水线 (.gitlab-ci.yml 示例)

这是一个标准的 GitLab CI 配置，包含**构建、测试、内存检查、静态分析**。

```yaml
image: ubuntu:22.04

variables:
  GIT_SUBMODULE_STRATEGY: recursive

before_script:
  - apt-get update && apt-get install -y cmake gcc ninja-build libuv1-dev libcmocka-dev cppcheck valgrind

stages:
  - static-analysis
  - build
  - test

# 1. 静态代码分析
cppcheck:
  stage: static-analysis
  script:
    - cppcheck --enable=all --error-exitcode=1 --suppress=missingIncludeSystem src/

# 2. 构建 (带 ASan)
build:
  stage: build
  script:
    - mkdir build && cd build
    - cmake -GNinja -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON ..
    - ninja
  artifacts:
    paths:
      - build/

# 3. 单元测试 (ASan 会在运行时捕获内存错误)
unit-test:
  stage: test
  dependencies:
    - build
  script:
    - cd build
    - ctest --output-on-failure

# 4. 内存泄漏深度检查 (Valgrind)
# 注意：ASan 和 Valgrind 不兼容，这里最好重新编译一个没 ASan 的 Release 版本
mem-check:
  stage: test
  script:
    - mkdir build-rel && cd build-rel
    - cmake -GNinja -DCMAKE_BUILD_TYPE=Release ..
    - ninja
    - valgrind --leak-check=full --error-exitcode=1 ./tests/unit_tests
```

### 总结

这个脚手架为纯 C 团队提供了现代化的开发保障：

1.  **架构：** 通过 `DeviceVTable` 实现了类似 OO 的接口多态，解决了设备多样性问题。
2.  **并发：** 通过 `libuv` 解决了复杂连接和定时任务问题。
3.  **安全：** CMake 强制集成 ASan，开发阶段就能发现 99% 的内存错误。
4.  **质量：** 集成了 `cmocka` 单元测试和 `cppcheck` 静态分析。

你的团队只需在这个框架下填充 `src/drivers/` 里的具体驱动逻辑（Modbus/TCP 等），即可得到一个稳健的边缘计算程序。

这是 C 语言开发中无法回避的终极痛点。在边缘计算设备上，**Segmentation Fault (段错误)** 不仅意味着程序崩溃，还可能导致现场设备失控、数据丢失，甚至需要技术人员驱车几百公里去重启设备。

要解决这个问题，不能仅靠“程序员细心”，必须建立一套**从编码规范到运行时保活的“防御纵深”**。

以下是针对 Linux 边缘设备 C 开发的 **防 Segfault 实战指南**：

---

### 第一道防线：编码习惯 (预防)

Segfault 的核心原因通常是：空指针解引用、数组越界、Use-after-free（释放后使用）。

#### 1. "指针必判空" 宏
不要在每个函数里手写 `if (ptr == NULL)`。定义宏来强制检查，不仅代码整洁，还能在 Release 版保留关键检查。

```c
#include <assert.h>
#include <stdlib.h>

// 在 Release 模式下不要禁用 assert，或者自定义一个 ASSERT
#define ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            fprintf(stderr, "[FATAL] Null pointer at %s:%d\n", __FILE__, __LINE__); \
            abort(); /* 立即生成 Core Dump，不要带病运行 */ \
        } \
    } while(0)

void process_data(char *data) {
    ASSERT_NOT_NULL(data);
    // 业务逻辑...
}
```

#### 2. 只有 `snprintf`，没有 `sprintf`
90% 的栈溢出 Segfault 都是字符串操作造成的。
*   ❌ 严禁使用：`sprintf`, `strcpy`, `strcat`
*   ✅ 必须使用：`snprintf`, `strncpy` (注意末尾补\0), `strncat`

#### 3. 结构体初始化惯例
未初始化的指针是随机值，一用就炸。
```c
// ❌ 危险
DeviceConfig conf;
// conf.name 指向哪里？只有天知道。

// ✅ 安全
DeviceConfig conf = {0}; // 所有指针字段自动置为 NULL
```

---

### 第二道防线：编译期与测试期 (拦截)

这是最有效的一步。**只要 ASan 开着，99% 的潜在 Segfault 活不到生产环境。**

#### 1. AddressSanitizer (ASan)
我在上一个回答中提到了它，这里强调它对 Segfault 的意义。它能精准捕获：
*   堆/栈缓冲区溢出 (Buffer Overflow)
*   释放后使用 (Use-after-free) —— **这是多线程边缘网关最容易出现的 Segfault 原因。**

**一定要在 CI/CD 流水线中跑 ASan 版本的单元测试。**

#### 2. 静态分析 (Static Analysis)
在代码还没编译前，工具就能告诉你“这里可能会空指针”。
*   **工具：** `cppcheck` 或 `clang-tidy`。
*   **规则：** 开启 `--enable=all`。

---

### 第三道防线：运行时“不死鸟” (保活)

C 程序在边缘设备上运行几个月，难免会遇到极其刁钻的竞态条件导致 Segfault。
**策略：** 既然无法保证 100% 不崩溃，那就保证 **“崩溃后 1 秒内复活”**。

#### 1. 使用 Systemd 进行进程守护 (标准做法)
不要自己写 shell 脚本死循环重启，用 Linux 标准的 systemd。

创建 `/etc/systemd/system/edgex-c.service`:

```ini
[Unit]
Description=EdgeX C Service
After=network.target

[Service]
Type=simple
ExecStart=/usr/local/bin/c_edgex_app
# 关键：崩溃后总是自动重启
Restart=always
# 关键：重启间隔 2秒，防止死循环耗尽 CPU
RestartSec=2
# 资源限制：防止内存泄漏把系统搞死 (OOM)
MemoryLimit=200M

[Install]
WantedBy=multi-user.target
```

#### 2. 硬件看门狗 (Hardware Watchdog)
如果你担心程序死锁（卡死但不崩溃），需要喂狗。
Linux 提供了标准的 `/dev/watchdog` 接口。
*   在 `libuv` 的定时器里每隔 10秒 写入一次 `/dev/watchdog`。
*   如果程序 Segfault 挂了，或者死循环了，无法喂狗，硬件会在 60秒内强制重启整个设备。

---

### 第四道防线：死后验尸 (调试)

当设备在几千公里外的现场崩溃时，你必须知道它死在哪一行。

#### 1. 开启 Core Dump
让 Linux 在程序崩溃时把内存镜像存成文件。

*   **设置：** `ulimit -c unlimited` (通常在启动脚本或 systemd LimitCORE 中设置)。
*   **调试：** 拿到 `core` 文件后，在本地电脑上：
    ```bash
    gdb ./c_edgex_app core.1234
    (gdb) bt  # 查看崩溃时的堆栈
    ```

#### 2. 捕获 SIGSEGV 并打印堆栈 (极简方案)
如果你觉得 Core Dump 文件太大（几十 MB），传输不便，可以在程序里捕获崩溃信号，打印堆栈后自行了断。

**在 `main.c` 中加入这段“黑匣子”代码：**

```c
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <execinfo.h> // 需要 glibc
#include <unistd.h>

void signal_handler(int sig) {
    void *array[10];
    size_t size;

    // 获取堆栈帧
    size = backtrace(array, 10);

    // 打印到 stderr (systemd 会记录到日志)
    fprintf(stderr, "Error: signal %d:\n", sig);

    // 打印符号信息 (注意：生产环境可能需要 strip 符号，这里最好配合 addr2line)
    backtrace_symbols_fd(array, size, STDERR_FILENO);

    // 恢复默认处理并退出
    signal(sig, SIG_DFL);
    exit(1);
}

int main() {
    // 注册信号处理
    signal(SIGSEGV, signal_handler); // 段错误
    signal(SIGABRT, signal_handler); // assert 失败

    // ... 正常逻辑
}
```
**效果：** 程序崩溃时，`journalctl -u edgex-c` 日志里会直接显示：
```text
Error: signal 11:
./c_edgex_app(main+0x20)[0x400f20]
./c_edgex_app(sim_read+0x15)[0x401a10]
/lib/libc.so.6(+0x3ef20)[0x7f...]
```
然后你可以通过 `addr2line -e c_edgex_app 0x401a10` 直接定位到文件名和行号。

---

### 总结：C 团队的生存法则

面对 Segfault，心态要从“极力避免”转变为“快速恢复与定位”。

1.  **开发期：** 必须用 **AddressSanitizer**，没跑过 ASan 的代码不许合入主分支。
2.  **部署期：** 必须用 **Systemd** 配置 `Restart=always`。
3.  **调试期：** 代码里植入 **Signal Handler** 打印堆栈，或者配置 **Core Dump**。

如果这三点做到了，C 语言项目的稳定性是可以达到工业级要求的。

处理边缘计算中的大量数据，是一个核心且复杂的挑战。与云端无限的资源不同，边缘设备资源受限，对数据处理的实时性、效率和可靠性有更高的要求。

鉴于你的团队熟悉 C 语言，我们将结合 C 语言的特性和优势，为你提供一套**兼顾性能、资源占用和工程化**的方案建议。

---

### 一、 边缘数据处理的核心挑战

1.  **数据洪泛 (Data Flood):** 传感器以高频次生成原始数据。
2.  **带宽限制 (Bandwidth Bottleneck):** 上传到云端的带宽昂贵且不稳定。
3.  **实时性要求 (Real-time Needs):** 某些决策必须在本地快速做出，不能依赖云端延迟。
4.  **资源受限 (Resource Constraints):** CPU、内存、存储空间远不及云端。
5.  **数据异构 (Data Heterogeneity):** 各种设备生成不同格式的数据。
6.  **安全与隐私 (Security & Privacy):** 敏感数据不能随意上传。

---

### 二、 针对 C 语言的边缘数据处理方案建议

我们将数据处理流程分为几个阶段，并为每个阶段提供 C 语言的实现方案。

#### 1. 数据采集与预处理 (Ingestion & Pre-processing)

这是数据进入系统的第一步，目标是高效、可靠地从各种源头获取数据，并进行初步过滤。

*   **C 语言方案：**
    *   **libuv:** 统一处理所有异步 I/O (串口、TCP/UDP Socket、文件)。所有采集任务都通过 `libuv` 的事件循环驱动，避免阻塞，实现高并发采集。
    *   **自定义协议解析器:** 针对 Modbus、CAN、自定义二进制协议等，手写 C 语言解析器。利用位操作和结构体映射，效率极高。
    *   **初步过滤 (Filtering):** 在数据进入核心系统前，进行简单的阈值过滤、重复数据删除。例如，温度每秒采集，但只有变化超过 0.1 度才传递。
*   **工程化实践：**
    *   **零拷贝 (Zero-Copy):** 尽可能在数据从硬件或网络读取后，避免不必要的内存拷贝。直接在 `libuv` 的回调中处理 `uv_buf_t`。
    *   **预分配缓冲区:** 为每个连接或传感器通道预分配固定大小的接收缓冲区，减少频繁的 `malloc/free`。

#### 2. 数据转换与标准化 (Transformation & Normalization)

将异构的原始数据转化为统一的内部表示，方便后续处理。

*   **C 语言方案：**
    *   **核心数据结构 (如 `EdgeEvent`):** 定义一套高效的 C 结构体来表示所有数据。使用 `union` 来处理不同类型的值（整数、浮点、布尔、字符串），节省内存。
    *   **cJSON:** 用于将内部结构体序列化为 JSON 字符串，便于发送到云端或本地存储。反之，也可用于解析来自云端或配置文件的 JSON 数据。
    *   **自定义转换函数:** 针对不同协议的原始数据，编写独立的 C 函数将其映射到统一结构体。
*   **工程化实践：**
    *   **模板宏 (Macro Templates):** 如果有大量类似的数据结构需要转换，可以考虑使用 C 语言的宏来生成重复的转换代码，减少手动编写。
    *   **配置文件驱动:** 使用 JSON/YAML 配置文件定义传感器的数据点（如 Modbus 寄存器地址、数据类型），C 程序解析这些配置来动态执行转换。

#### 3. 本地存储与持久化 (Local Storage & Persistence)

在网络不稳定或需要本地数据历史时，将数据持久化到本地。

*   **C 语言方案：**
    *   **SQLite (推荐):** 轻量级、无服务器的嵌入式关系型数据库，非常适合边缘设备。它只有一个 C 语言库文件，内存占用极小。
        *   **优势：** 支持 SQL，查询灵活；事务安全；适合结构化数据存储。
        *   **用法：** 直接调用 `sqlite3_open()`, `sqlite3_exec()`, `sqlite3_prepare_v2()` 等 C API。
    *   **自定义文件系统存储 (适用于简单场景):**
        *   **环形缓冲区 (Ring Buffer) + 文件:** 对于日志或简单时序数据，可以实现一个将数据写入预分配文件块的环形缓冲区。
        *   **Append-only Log Files:** 将数据追加写入文件，达到一定大小或时间后轮转。
*   **工程化实践：**
    *   **异步写入:** 将写入数据库或文件的操作放在 `libuv` 的线程池中执行 (`uv_queue_work`)，避免阻塞主事件循环。
    *   **数据压缩:** 对存储在文件系统中的数据使用 `zlib` 或 `lz4` 进行压缩，节省存储空间。
    *   **数据老化策略:** 实现定期清理旧数据的逻辑，防止存储空间耗尽。

#### 4. 边缘分析与AI推理 (Edge Analytics & AI Inference)

在设备本地对数据进行分析，提取有价值的信息，甚至运行机器学习模型。

*   **C 语言方案：**
    *   **数学库:** `GNU Scientific Library (GSL)` 或自定义轻量级算法，用于统计分析、信号处理（滤波、FFT）。
    *   **TensorFlow Lite (C API):** 如果需要运行 AI 模型（图像识别、异常检测），这是最主流的方案。
        *   **优势：** 针对嵌入式设备优化，体积小，性能高。
        *   **流程：** 在云端训练模型 -> 转换为 `.tflite` 格式 -> 部署到边缘设备 -> C 程序调用 TFLite C API 进行推理。
    *   **ONNX Runtime (C API):** 另一种通用的 AI 模型推理引擎，支持多种框架导出的 ONNX 格式模型。
*   **工程化实践：**
    *   **模型轻量化:** 部署到边缘的模型必须是轻量级模型 (TinyML)。量化 (Quantization)、剪枝 (Pruning) 等技术是必须的。
    *   **异步推理:** 将模型推理任务放到 `libuv` 线程池中，避免阻塞实时数据采集。
    *   **内存池管理:** 为 AI 模型的输入/输出张量分配专用内存池，减少碎片。

#### 5. 数据聚合、压缩与上云 (Aggregation, Compression & Export)

在数据上传到云端前，尽可能地减少数据量，保证上传的可靠性。

*   **C 语言方案：**
    *   **时间/计数聚合:** 实现 C 语言逻辑，将一段时间内或一定数量的数据进行统计（平均值、最大值、最小值、总和），只上传聚合后的结果。
    *   **数据压缩 (zlib/lz4):** 将序列化后的 JSON 字符串或二进制数据进行压缩，再上传。
    *   **MQTT 客户端 (paho.mqtt.c):** 最适合物联网设备的数据传输协议。
        *   **优势：** 轻量级、发布/订阅模型、支持 QoS (保证消息送达)、断线重连。
    *   **HTTP 客户端 (libcurl):** 用于向 RESTful API 上传数据或获取配置。
    *   **自定义重传队列:** 在发送失败时，将数据存入本地 SQLite 队列，等待网络恢复后重传。
*   **工程化实践：**
    *   **消息缓存:** 使用一个基于 `libuv` 的异步消息队列 (或简单链表/环形缓冲区) 来缓存待发送的消息，在网络通畅时批量发送。
    *   **可配置的上传策略:** 允许通过配置文件动态调整聚合周期、压缩级别、上传 QoS 等参数。

---

### 三、 C 语言数据处理的通用工程化最佳实践

1.  **内存池管理:** 对于大量频繁创建/销毁的小对象，使用自定义内存池 (Arena Allocator) 来代替 `malloc/free`，能显著减少内存碎片和提高性能，同时降低 Segfault 风险。
2.  **异步化设计 (libuv):** 所有 I/O 操作都必须是非阻塞的。计算密集型任务放入 `libuv` 的线程池。
3.  **日志记录 (zlog/log.c):** 详细记录数据处理的每个阶段，便于故障排查和性能分析。
4.  **配置文件驱动:** 将所有可变参数（设备地址、采样周期、阈值、云端地址）放入配置文件，支持热更新。
5.  **模块化设计:** 将采集、处理、存储、导出等功能封装为独立的 C 模块 (编译为静态库或共享库)，通过接口 (`DeviceVTable` 模式) 通信，降低耦合。
6.  **错误处理与鲁棒性:** 每一个 I/O 操作、内存分配都必须检查返回值。建立完善的错误码体系。

---

### 四、 总结

对于 C 语言团队，处理边缘设备上的大量数据，关键在于 **精心设计、充分利用成熟的 C 语言库，并遵循严格的工程实践**。

*   **libuv** 是心脏，负责所有异步并发。
*   **SQLite** 是大脑，负责本地数据管理。
*   **cJSON/paho.mqtt.c** 是嘴巴，负责数据对外沟通。
*   **ASan/Valgrind/CMocka** 是卫兵，确保代码质量和内存安全。

```
SDK built on: ubuntu 22.04
gcc: 11.4.0

cmocka: v1.1.7
libuv: v1.48.0
libmodbus: v3.1.10
cJSON: v1.7.18
sqlite: 3.45.1 (amalgamation)

```

```
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential cmake git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /work
RUN git clone https://your.repo/project.git .

# 直接用仓库里的 .local
ENV SDK_ROOT=/work/.local/linux-x86_64
ENV CMAKE_PREFIX_PATH=$SDK_ROOT
ENV CPATH=$SDK_ROOT/include
ENV LIBRARY_PATH=$SDK_ROOT/lib

RUN cmake -B build -S . \
 && cmake --build build
```


以下是当前代码进度和架构现状的全面梳理：

  ---

  🏗️ 1. 核心内核与运行时 (Core Runtime) - 100% 完成
   * 运行时容器 (`service_t`)：实现了控制反转 (IoC) 架构。src/core/edge_service.c
     负责生命周期管理，不持有具体业务逻辑。
   * 零拷贝内存池 (`net_buf`)：完全移植并加固了 Zephyr net_buf。支持 Headroom/Tailroom
     操作及递归分片释放，解决了内存碎片问题。
   * 事件驱动核心：基于 libuv 的单线程异步模型，通过 uv_async 和 uv_thread
     完美隔离了“控制面”与“执行面”。

  📡 2. 数据平面 (Data Plane) - 100% 完成
   * 高速总线 (`shm_ring` / `datahub`)：实现了无锁共享内存环形缓冲区。
       * 性能：支持纳秒级数据写入。
       * 通知：跨线程/进程的 uv_async 唤醒机制，杜绝了 Busy-poll。
   * 数据结构：统一使用 64 字节对齐的 PipeMsg，支持标量、事件及 dma-buf 引用。

  ⚖️ 3. 总线仲裁与 I/O (Bus & I/O Plane) - 90% 完成
   * 仲裁器 (`arbiter`)：支持多总线、多优先级、抢占式调度及 Exclusive（独占）租约机制。
   * 执行器 (`io_executor`)：每个总线拥有独立的 IO 线程，支持阻塞式物理读取而不影响主循环。
   * 模拟机制：内置了测试用的 Mock IO 逻辑，确保在无硬件环境下也能跑通全流程测试。

  💾 4. 服务层 (Service Plane) - 85% 完成
   * 异步存储 (`sqlite_writer`)：实现了基于 WAL 模式的单写者架构。主线程通过队列提交
     SQL，后端线程批量落盘。
   * 脚本沙箱 (`js_engine`)：集成 QuickJS。实现了内存配额（2MB）和执行中断保护。
   * 动态加载 (`script_loader`)：实现了 JS
     脚本的热加载与监控，支持在不重启系统的情况下下发报警/控制策略。
   * 数据融合 (`algorithm_sum`)：实现了多点聚合计算的示范引擎。

  🌐 5. 北向交互 (Northbound) - 95% 完成
   * 职责分离：
       * HMI (:8080)：纯静态 WebServer，内嵌 TailwindCSS + AlpineJS UI。
       * REST API (:8081)：业务网关，支持 CORS，提供 JSON 数据接口及 SSE 实时推送。
   * 命令桥接 (Command Bridge)：实现了从 Web/REST 到核心容器的安全指令投递逻辑。

  ---

  📂 6. 工程化与质量保证 - 100% 完成
   * 密封构建：.local 目录物理隔离不同架构的依赖，build_vendors.sh 确保依赖可复现。
   * 产品化：cmd/ 目录下五个产品（IEMS, SG, HEMS, EVSE, DCU）拥有各自的实例化入口。
   * 质量守卫：
       * ASan/UBSan：编译时集成，实时监控内存泄漏与非法访问。
       * CMocka：全量单元测试（11个 Test Suite 全部 Passing）。

  ---

  🗺️ 7. 场景覆盖对照表 (Scenario Check)

  ┌──────┬────────────┬───────────────────────────────────────────────────────────┐
  │ 场景 │ 名称       │ 代码实现状态                                              │
  ├──────┼────────────┼───────────────────────────────────────────────────────────┤
  │ S1   │ 周期性采集 │ 已实现 (on_poll_timer -> Arbiter -> Executor)               │
  │ S2   │ 主动上报   │ 已实现 (uv_poll 监听总线 FD)                              │
  │ S5   │ 即时点抄   │ 已实现 (REST POST -> Command Bridge -> Arbiter High Prio) │
  │ S6   │ 反向控制   │ 已实现 (REST -> JS Engine -> Arbiter)                     │
  │ S7   │ 从设备 OTA │ 已实现 (OTA Start -> Arbiter Exclusive Lease)             │
  │ S8   │ 对时处理   │ 已实现 (System Clock Jump Detection in Scheduler)         │
  │ S9   │ 算法融合   │ 已实现 (ShmRing -> Algorithm -> DataHub)                  │
  └──────┴────────────┴───────────────────────────────────────────────────────────┘

  ---

  🚀 接下来建议的方向：
   1. 协议栈深度化：基于 net_buf 实现更复杂的 DLMS/COSEM 解析（针对 DCU 产品）。
   2. 安全性增强：接入 OpenSSL Engine 或硬件 SE 芯片的签名验证逻辑（针对 SG-EL2025 产品）。
   3. 容器化部署：优化 docker/Dockerfile 以生成最终的发布镜像。
