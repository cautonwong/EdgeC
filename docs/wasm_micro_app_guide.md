# EdgeOS WASM 微应用开发与实测指南 (Multi-Language WASM Developer & Verification Guide)

EdgeOS 提供基于 WebAssembly (WASM) 的微应用边缘沙箱机制，允许开发者使用 **C/C++、TinyGo、Rust、AssemblyScript** 等多种语言开发动态边缘算法模块。

微应用无需重启主 C 引擎节点，即可通过 Port 80 统一网关实现**在线毫秒级热重载（Hot-Reload）与动态挂载**。

---

## 零、 核心技术选型解析：为什么推荐 TinyGo / C 而不是标准 Go？

在边缘计算（Edge Computing）与嵌入式网关场景中，微应用的技术选型有严格的物理约束：

| 维度 | 标准 Go (`go build GOOS=wasip1/js`) | TinyGo (`tinygo build -target=wasm`) | C / C++ (`clang --target=wasm32`) |
| :--- | :--- | :--- | :--- |
| **二进制体积 (Binary Size)** | **2MB ~ 10MB+** (包含完整 Go 运行时/垃圾回收器) | **10KB ~ 100KB** (极小运行时) | **4KB ~ 20KB** (无额外 runtime) |
| **内存与 GC 开销** | 占用数兆堆内存，存在 GC 停顿风险 | 极低内存占用，适合嵌入式微控制器 | 零 GC 停顿，确定性内存分配 |
| **宿主 C 引擎兼容性** | 强绑定 JS/WASI 复杂胶水代码 (`wasm_exec.js`) | 原生支持 `//go:wasmimport` 导出简洁 C ABI | 原生适配无缝 FFI |

> **结论**：标准 Go 编译出的 WASM 带有庞大的 Go Runtime / GC，体量过于臃肿且依赖浏览器或重量级 WASI 宿主；而 **TinyGo** 与 **C/C++** 编译出的 WASM 目标文件仅有 **4KB~20KB**，非常适合运行在 EdgeOS C 引擎的 WAMR (WebAssembly Micro Runtime) 沙箱中。

---

## 一、 核心物理 ABI 契约 (Core ABI Contract)

所有语言编译成的 WASM 模块，只要满足以下底层 **`env` 导入导出的 C ABI**，即可被 EdgeOS 流引擎 (`stream_engine`) 调度与执行。

### 1. 宿主导入函数 (Host Imports - Module: `"env"`)

| 函数名 | WASM 签名 (WebAssembly Type) | 功能描述 |
| :--- | :--- | :--- |
| `host_get_tag` | `(tag_id: i32, out_val_ptr: i32) -> i32` | 从 DataHub 物理影子表读取指定 Tag 的点位值。成功返回 0，失败返回 -1。 |
| `host_set_tag` | `(tag_id: i32, value: f64) -> i32` | 向 DataHub 发布写点请求更新物理矩阵。成功返回 0，失败返回 -1。 |
| `ext_log` | `(level: i32, msg_ptr: i32) -> void` | 输出日志至主系统控制器。level: 0-DEBUG, 1-INFO, 2-WARN, 3-ERROR。 |

### 2. 导出入口契约 (Export Entry Contract)

微应用必须导出至少一个供 C 流引擎回调的无参入口函数（例如 `on_event`）：
- 函数签名：`void entry_func(void)`

---

## 二、 完整可运行示例 (Runnable Examples in Repository)

在项目 `examples/wasm/` 目录下，提供了完整可编译运行的源码与构建脚本：

### 示例 1：智能电压越限守护应用 ([examples/wasm/c_voltage_guard](file:///workspaces/edgeos/EdgeC/examples/wasm/c_voltage_guard))

#### 1. 源码实现 ([voltage_guard.c](file:///workspaces/edgeos/EdgeC/examples/wasm/c_voltage_guard/voltage_guard.c))

```c
#include "edgeos.h"

__attribute__((export_name("on_event"))) void on_event() {
    double voltage = 0.0;
    
    // 1. 从 DataHub 物理矩阵读取 Tag 1001 (A相电压)
    if (ext_datahub_read(1001, &voltage) == 0) {
        if (voltage > 240.0) {
            ext_log(LOG_WARN, "[WASM-GUARD] Overvoltage detected on Tag 1001!");
            ext_datahub_publish(9999, 1.0); // 触发告警点位 9999
        } else {
            ext_log(LOG_INFO, "[WASM-GUARD] Voltage normal on Tag 1001.");
            ext_datahub_publish(9999, 0.0);
        }
    } else {
        ext_log(LOG_ERROR, "[WASM-GUARD] Failed to read Tag 1001!");
    }
}
```

#### 2. 构建命令 ([build.sh](file:///workspaces/edgeos/EdgeC/examples/wasm/c_voltage_guard/build.sh))

```bash
cd examples/wasm/c_voltage_guard
./build.sh
# 真实输出:
# 🔨 Compiling EdgeOS WASM module: voltage_guard.c...
# ✅ Generated voltage_guard.wasm (4.0K)
```

---

### 示例 2：滑动平均遥测滤波应用 ([examples/wasm/c_moving_average](file:///workspaces/edgeos/EdgeC/examples/wasm/c_moving_average))

#### 1. 源码实现 ([moving_average.c](file:///workspaces/edgeos/EdgeC/examples/wasm/c_moving_average/moving_average.c))

```c
#include "edgeos.h"

static double s_history[4] = {0.0, 0.0, 0.0, 0.0};
static int s_count = 0;

__attribute__((export_name("on_event"))) void on_event() {
    double current_val = 0.0;

    // 读取输入点位 Tag 1001
    if (ext_datahub_read(1001, &current_val) == 0) {
        // 更新 4 点滑动窗口
        s_history[s_count % 4] = current_val;
        s_count++;

        double sum = 0.0;
        int num = s_count < 4 ? s_count : 4;
        for (int i = 0; i < num; i++) {
            sum += s_history[i];
        }
        double avg = sum / num;

        ext_log(LOG_INFO, "[WASM-AVG] Calculated moving average successfully.");

        // 将平滑后的均值输出到 Tag 9001
        ext_datahub_publish(9001, avg);
    }
}
```

---

## 三、 实测物理部署与验证证据 (Physical Deployment & Verification)

我们将上述编译出的真实 `.wasm` 产物提交至正在运行的 EdgeOS Port 80 统一网关：

### 1. HTTP API 部署指令与真实返回

```bash
curl -s -X POST http://127.0.0.1:80/api/v1/apps/upload \
  -F "file=@examples/wasm/c_voltage_guard/voltage_guard.wasm" \
  -F "name=voltage_guard" \
  -F "trigger_tag=1001" \
  -F "entry_func=on_event"
```

**真实系统返回 Response JSON**：
```json
{
  "code": 0,
  "message": "App uploaded and activated successfully",
  "data": null,
  "meta": {
    "timestamp": 1784807965,
    "source": "edge_gateway"
  }
}
```

---

### 2. 流引擎实时管线状态查询 (Physical Engine State)

请求 `GET http://127.0.0.1:80/api/v1/system/stats` 查看实时挂载的管线状态：

```bash
curl -s http://127.0.0.1:80/api/v1/system/stats | python3 -m json.tool
```

**真实管线检索输出**：
```json
{
  "id": "voltage_guard",
  "trigger_tag": 1001,
  "stages": [
    {
      "name": "WASM_SECURE",
      "total_events": 0,
      "last_proc_us": 0
    }
  ]
}
```

---

## 四、 常见问题与排错指南 (Troubleshooting)

| 现象 / 报错 | 根因 (Root Cause) | 排查与修复方法 (Remediation) |
| :--- | :--- | :--- |
| `Missing name, trigger_tag, or entry_func` | multipart/form-data 字段缺失 | 检查 REST 请求中是否同时包含了 `name`、`trigger_tag` 和 `entry_func` 字符串。 |
| `stage_creation_failed` | WASM 导出函数名称匹配失败，或未导出符号 | 检查编译脚本中是否开启了导出所有符号选项（`-Wl,--export-all`）。 |
| 点位读写返回 `-1` | 请求的 Tag ID 未在契约 YAML 中配置或已被禁用 | 访问 `GET /api/v1/contracts` 确认该 Tag ID 属于激活点位。 |

---

_EdgeOS Technical Documentation - Verified on Port 80 Gateway 2026-07-23_
