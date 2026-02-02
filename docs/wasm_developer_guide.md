# 🚀 EdgeOS WASM 算子开发指南 (V1.0)

WebAssembly (WASM) 在 EdgeOS 中作为**高性能动态算法载体**，允许你在不重启系统的情况下，下发、更新和运行 C/C++/Rust 编写的复杂逻辑。

## 1. 开发环境准备

### 1.1 编译器要求
EdgeOS 使用 **WAMR (WebAssembly Micro Runtime)**。推荐使用 `clang` 直接编译，无需复杂的 WASI-SDK。
*   **Ubuntu/Debian:** `sudo apt install clang lld`
*   **验证:** `clang --version` (建议 12.0+)

### 1.2 目录结构
建议在 `algo/wasm/` 目录下创建你的算子文件夹：
```bash
algo/wasm/
└── my_awesome_algo/
    ├── edgeos.h      # 从系统 include 拷贝或引用
    ├── main.c        # 你的逻辑
    └── build.sh      # 编译脚本
```

---

## 2. 编写 WASM 逻辑

### 2.1 引入 ABI 接口 (`edgeos.h`)
WASM 运行在沙箱中，必须通过 EdgeOS 提供的 **ABI (Application Binary Interface)** 与宿主通信。请确保你的代码包含以下定义：

```c
#ifndef WASM_EDGEOS_H
#define WASM_EDGEOS_H

#include <stdint.h>

// 1. 读取 DataHub 测点快照 (从 Shadow Table 读取最新值)
__attribute__((import_module("env"), import_name("ext_datahub_read"))) 
extern int ext_datahub_read(uint32_t tag_id, double *out_val);

// 2. 发布数据到 DataHub (推送到共享内存环)
__attribute__((import_module("env"), import_name("ext_datahub_publish"))) 
extern int ext_datahub_publish(uint32_t tag_id, double value);

// 3. 日志打印 (输出到 EdgeOS 系统日志)
__attribute__((import_module("env"), import_name("ext_log"))) 
extern void ext_log(int level, const char *msg);

#define LOG_DEBUG 0
#define LOG_INFO 1
#define LOG_WARN 2
#define LOG_ERROR 3

#endif
```

### 2.2 实现业务逻辑 (`main.c`)
创建一个导出函数（通常名为 `on_event`），作为 Pipeline 触发时的入口。

```c
#include "edgeos.h"
#include <stdio.h>

// 使用 export_name 确保函数在 WASM 模块外可见
__attribute__((export_name("on_event"))) 
void on_event() {
    double voltage = 0;
    
    // 读取 1001 号点位 (如 A 相电压)
    if (ext_datahub_read(1001, &voltage) == 0) {
        // 业务逻辑：如果电压 > 240V，触发 9999 号点位报警
        if (voltage > 240.0) {
            ext_log(LOG_WARN, "WASM: [Alert] Overvoltage detected!");
            ext_datahub_publish(9999, 1.0);
        } else {
            ext_datahub_publish(9999, 0.0);
        }
    } else {
        ext_log(LOG_ERROR, "WASM: Failed to read Tag 1001");
    }
}
```

---

## 3. 编译为 WASM

使用 `clang` 将 C 代码编译为独立的 WASM 二进制文件。

### 3.1 编译命令模板 (`build.sh`)
```bash
#!/bin/bash
CLANG=clang
OUTPUT="my_algo.wasm"
SOURCE="main.c"

$CLANG --target=wasm32 \
    -O3 \
    -nostdlib \
    -Wl,--no-entry \
    -Wl,--export-all \
    -Wl,--allow-undefined \
    -o $OUTPUT $SOURCE

echo "✅ Generated $OUTPUT"
```
**关键参数解释：**
*   `--target=wasm32`: 目标架构。
*   `-nostdlib`: 不链接标准库（减小体积，边缘端算法通常不需要全量 libc）。
*   `-Wl,--no-entry`: 没有 `main` 函数入口。
*   `-Wl,--allow-undefined`: 允许 ABI 接口在编译时未定义（由 EdgeOS 运行时链接）。

---

## 4. 在 EdgeOS 中集成

### 4.1 配置文件挂载 (`pipelines.yaml`)
在产品的契约文件或配置文件中，添加一个类型为 `wasm_executor` 的 Stage。

```yaml
pipelines:
  - id: "wasm_protection"
    trigger: 1001          # 1001 点位有数据时触发
    stages:
      - type: "wasm_executor"
        config:
          wasm_path: "/opt/edgeos/algo/my_algo.wasm"
          entry_func: "on_event"
```

### 4.2 运行时内存说明
*   **Stack Size:** 默认 64KB。
*   **Heap Size:** 默认 64KB。
如果你的算法需要处理大数据块，请在 `src/stream_engine/wasm_runtime/wasm_mgr.c` 中调整 `wasm_runtime_instantiate` 参数。

---

## 5. 调试与验证

### 5.1 查看日志
WASM 内部调用的 `ext_log` 会直接映射到系统日志，你可以通过以下方式查看：
```bash
tail -f /var/log/edgeos.log | grep "[WASM]"
```

### 5.2 性能监控
EdgeOS 会统计每个 Pipeline Stage 的耗时。
*   **AOT 优化:** 如果发现解释执行性能不足，可以使用 `wamrc` 将 `.wasm` 编译为 `.aot`。
    ```bash
    iwasm-2.4.3/wamrc -o my_algo.aot my_algo.wasm
    ```
    然后在配置文件中将 `wasm_path` 指向 `.aot` 文件。

---

## 6. 开发者避坑指南 (FAQ)

1.  **Q: 为什么 `ext_datahub_read` 读不到数据？**
    *   **A:** 该接口读取的是 **Shadow Table**。请确保该测点已经通过 `datahub_publish` 发布过，且 DataHub 初始化时分配的影子表容量（`shadow_capacity`）大于测点的 `tag_id`。
2.  **Q: 出现 `failed to check signature` 错误？**
    *   **A:** 确保 WASM 中的函数声明与宿主 C 代码中的 `NativeSymbol` 签名匹配。EdgeOS 目前对指针（字符串/数组）统一使用 `uint32_t` 映射。
3.  **Q: 段错误 (Segmentation Fault)？**
    *   **A:** 检查 WASM 是否试图访问沙箱外的地址。所有指针必须通过 ABI 接口传递，不能直接解引用宿主内存地址。

```