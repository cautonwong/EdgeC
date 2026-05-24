# EdgeC 模块化重构计划 (Qt-Style Architecture)

## 1. 架构愿景：为什么我们要学 Qt？

目前 EdgeC 的代码集中在 `src/core`、`src/main_loop` 和 `src/stream_engine` 中，所有的文件都在根目录的 `CMakeLists.txt` 中被编译为一个或几个巨大的库（如 `edge_main_loop`、`edge_stream_engine`）。

**痛点：**
- **依赖乱锅**：底层核心（DataHub）和业务协议（Modbus）的头文件互相可见，稍不注意就会出现循环引用。
- **剪裁困难**：虽然有 Kconfig，但目前只能通过 `#ifdef` 在代码内部屏蔽逻辑，文件依然参与编译，不够优雅。
- **复用性差**：如果未来我们想单独把 `NetBuf` 或者 `DataHub` 拿出来给别的项目用，会发现它绑架了一堆无关的代码。

**Qt 的解药（Target-based 模块化）：**
我们将采用现代 CMake 的 **Target-based** 设计。系统分为多个独立的模块（库），每个模块只对外暴露自己想暴露的头文件。**下层对上层绝对无知，上层通过 `target_link_libraries` 显式依赖下层。**

---

## 2. 目标目录结构 (Package-Oriented)

重构后，`src/` 目录将按“包（Package）”或“模块”重新组织。每个子目录都是一个独立的 CMake Target。

```text
EdgeC/
├── modules/                         # 所有核心模块存放地
│   ├── edge_core/                   # 【基础底座】(对应 QtCore)
│   │   ├── CMakeLists.txt           # target: edge_core
│   │   ├── include/                 # 仅自身可见的私有头文件
│   │   └── src/                     # net_buf.c, datahub.c, arbiter.c, shmring.c
│   │
│   ├── edge_protocol/               # 【南/北向协议栈】(对应 QtNetwork)
│   │   ├── CMakeLists.txt           # target: edge_protocol (PUBLIC LINK edge_core)
│   │   ├── include/                 # 协议抽象层
│   │   ├── iec104/                  # iec104_apci.c, iec_asdu.c
│   │   ├── modbus/                  # modbus_codec.c
│   │   └── ccsds/                   # CCSDS, HL7 等
│   │
│   ├── edge_io/                     # 【I/O 调度与持久化】
│   │   ├── CMakeLists.txt           # target: edge_io (PUBLIC LINK edge_protocol)
│   │   └── src/                     # bus_orchestrator.c, driver_manager.c, snf_manager.c, sqlite_service.c
│   │
│   ├── edge_algo/                   # 【纯算法库】
│   │   ├── CMakeLists.txt           # target: edge_algo (PUBLIC LINK edge_core)
│   │   └── src/                     # matrix, control (PID, LQR), dsp (FFT)
│   │
│   ├── edge_security/               # 【安全套件】
│   │   ├── CMakeLists.txt           # target: edge_security (PUBLIC LINK edge_core)
│   │   └── src/                     # edge_guard_dpi.c, secure_syslog.c
│   │
│   └── edge_stream/                 # 【算子执行引擎】
│       ├── CMakeLists.txt           # target: edge_stream (PUBLIC LINK edge_core, edge_algo)
│       ├── pipeline/                # stream_engine.c, pipeline_loader.c
│       └── stages/                  # 所有具体的算子 (stage_compactor.c, stage_radar_tracker.c等)
│
├── include/                         # 【全局公开 API】(供外部 App 或第三方插件使用)
│   ├── edgeos.h                     # 类似 <QtWidgets/QtWidgets>，一键包含所有公开接口
│   ├── edge_types.h
│   └── edge_api.h
│
├── cmd/                             # 最终的应用程序 (iems, dcu, robot)
│   └── iems/
│       └── CMakeLists.txt           # target_link_libraries(iems PRIVATE edge_io edge_stream edge_security)
└── tests/                           # 单元测试
```

---

## 3. 核心机制：CMake 依赖可见性 (PUBLIC vs PRIVATE)

这是重构的灵魂。我们在每个 `modules/xxx/CMakeLists.txt` 中严格控制边界。

### 例子：`edge_protocol` 模块的 CMake 配置

```cmake
# modules/edge_protocol/CMakeLists.txt

add_library(edge_protocol STATIC
    modbus/modbus_codec.c
    iec104/iec104_apci.c
    iec104/iec_asdu.c
)

# 1. 声明协议模块的 PUBLIC 头文件目录
# 意思是：谁 link 了我，谁就能 include 这里的头文件
target_include_directories(edge_protocol PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 2. 声明 PRIVATE 头文件目录
# 意思是：只有我自己的 .c 文件能 include，外部看不见（防止污染）
target_include_directories(edge_protocol PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/modbus/priv
)

# 3. 声明依赖
# 协议层解析数据需要使用 NetBuf，所以必须依赖 edge_core。
# PUBLIC 意味着：链接 edge_protocol 的目标，也会自动链接 edge_core。
target_link_libraries(edge_protocol PUBLIC edge_core)
```

**效果：**
如果 `edge_algo` 模块只需要做数学运算，它只 link `edge_core`。那么在 `edge_algo` 的代码里 `#include "modbus_codec.h"` 会直接报 **编译错误 (File not found)**。从物理层面上杜绝了模块乱引用的可能。

---

## 4. 协议模块 (edge_protocol) 的特殊考量

协议处理是边缘网关最庞杂的部分。我们将把它设计成一个高度抽象的“编解码器工厂”。

### 抽象接口化 (Interface Pattern)
所有的协议不允许暴露具体的结构体细节给核心调度层，而是实现一个统一的接口表（类似 Linux 内核的 `file_operations`）：

```c
// include/edge_protocol/edge_codec.h

typedef struct edge_codec_ops_s {
    // 将收到的原始字节流解码为 PipeMsg (零拷贝)
    int (*decode_rx)(net_buf_t *rx_buf, PipeMsg *out_msg);
    
    // 将 PipeMsg 编码为具体的协议字节流发送
    int (*encode_tx)(const PipeMsg *msg, net_buf_t *tx_buf);
} edge_codec_ops_t;

// 各个协议对外只暴露注册接口
void modbus_codec_register(void);
void iec104_codec_register(void);
```

### 零拷贝传递
`edge_protocol` 解码出的 payload，如果是大型连续数据（如 CCSDS 的高速遥测，或雷达波形），不进行 `memcpy`。直接在 `PipeMsg` 中将 `payload.ref.ptr` 指向 `NetBuf` 的地址，并将 `NetBuf` 的引用计数加一，通过 `DataHub` 丢给 `edge_algo` 算子处理。

---

## 5. 保姆级重构实施步骤 (Step-by-Step)

为了保证系统始终可编译、可测试，我们必须采取“渐进式重构法”。

### Phase 1: 建立隔离区与底座搬迁
1.  在 `EdgeC` 下创建 `modules/edge_core/src` 和 `modules/edge_core/include`。
2.  将现有的 `net_buf.c/h`, `datahub.c/h`, `arbiter.c/h`, `shmring.c/h` 移动进去。
3.  编写 `modules/edge_core/CMakeLists.txt`。
4.  在根目录 `CMakeLists.txt` 中使用 `add_subdirectory(modules/edge_core)`。
5.  修改所有其他文件的 `#include` 路径，**编译并运行所有测试**。确保底座稳固。

### Phase 2: 剥离算法与安全模块 (无依赖的纯逻辑模块)
1.  创建 `modules/edge_algo`。将 `algo/` 目录下的内容整体迁入。
2.  创建 `modules/edge_security`。迁入 `edge_guard_dpi` 和 `secure_syslog`。
3.  为它们配置 CMake，并分别 `target_link_libraries(xxx PUBLIC edge_core)`。
4.  **编译并运行所有测试**。

### Phase 3: 重构协议模块 (最痛苦的阶段)
1.  创建 `modules/edge_protocol`。
2.  将 IEC104、Modbus、SparkplugB、CCSDS、HL7 的相关编解码代码迁入。
3.  **梳理头文件**：把对外部隐藏的协议内部常量（如 Modbus 寄存器地址表）移到 `PRIVATE` 目录，把必须公开的接口放到 `PUBLIC` 目录。
4.  如果有协议代码直接操作了 `DataHub`，**强行斩断**。协议只负责生成和解析 `NetBuf`，由调用者（IO 调度器）去把数据丢进 `DataHub`。

### Phase 4: 整合 I/O 与流水线
1.  创建 `modules/edge_io` (依赖 Core 和 Protocol)，迁入 `bus_orchestrator` 和 `driver_manager`。
2.  创建 `modules/edge_stream` (依赖 Core, Algo)，迁入所有的 `stage_*.c` 算子和 `stream_engine` 核心逻辑。
3.  清理根目录的 `include/`，只保留最终给用户 App (如 `iems`, `dcu`) 用的 `edge_api.h` 等全局总集头文件。

### Phase 5: Kconfig 细粒度分包 (Local Kconfig)
为了实现真正的模块化，我们不再使用单一的根目录 `Kconfig`，而是模仿 **Zephyr RTOS** 的做法，让配置定义也随包走：

1.  **分布式定义**：每个 `modules/xxx/` 目录下都建立一个属于自己的 `Kconfig` 文件，定义该模块特有的配置项。
2.  **层级包含**：根目录 `Kconfig` 仅负责 `source "modules/*/Kconfig"`。
3.  **CMake 联动**：在每个模块的 `CMakeLists.txt` 中，直接根据这些本地配置项来决定编译哪些文件。例如：
    ```cmake
    # modules/edge_protocol/CMakeLists.txt
    if(CONFIG_EDGEC_PROTO_IEC104)
        target_sources(edge_protocol PRIVATE iec104/iec104_apci.c)
    endif()
    ```
4.  **效果**：这样当 Kconfig 关闭某个协议时，连该协议的源文件都不会参与构建系统的扫描，实现“代码级”的深度剪裁。

---

## 6. 风险与缓解策略

1.  **风险：大量的 Include Path 错误。**
    *   *缓解*：每次移动一个模块，都必须跑一次 `ninja`。不要试图一次性移动所有文件。
2.  **风险：循环依赖暴露。**
    *   例如发现 A 依赖 B，B 又依赖 A。
    *   *缓解*：在重构时，遇到循环依赖，说明设计本身有问题。必须通过抽象出第三个共用结构体（下沉到 `edge_core`），或者通过回调函数（指针注入）来解耦。
3.  **风险：Git 历史追踪断裂。**
    *   *缓解*：移动文件时必须使用 `git mv`，而不是手动的复制粘贴，以保留文件的修改历史。

---
这份设计遵循了经典操作系统的包管理哲学。当完成这次重构后，EdgeC 将不再是一个单纯的程序，而是一个高度可复用、可剪裁的 **C 语言工业级中间件框架**。