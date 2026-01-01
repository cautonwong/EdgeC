# EdgeOS 开发者治理手册 (Industrial v2.0 - Frozen Edition)

> **修订说明：** 本文档不是一份简单的 README 扩展，它是 EdgeOS 的“工程定力之基”。
> 每一位参与本项目的开发者（包括核心库开发者与产品集成商）必须深刻理解并践行本手册定义的治理逻辑。
>
> **核心价值观：** 物理隔离、契约驱动、ABI 稳定、环境零污染。

---

## 一、 架构定力：四层逻辑平面的“物理生存空间”

**结论先行：**
**在 EdgeOS 中，平面的边界即是安全的红线。跨平面的非法耦合是导致系统在 10 年后变得不可维护的根源。**

我们为什么放弃单体库，宁愿增加链接复杂度也要拆分出四个物理库？

### 1.1 四层平面的深层治理逻辑

| 平面名称 | 归属目录 | 库产物 | 稳定性 | 治理目的 |
| :--- | :--- | :--- | :--- | :--- |
| **Foundation (L0)** | `include/` | `*.h` | **永恒** | **数据宪法**。仅定义类型，作为 L1/L2 的公共契约，打破依赖环。 |
| **Foundation (L1)** | `src/core/` | `libedge_core.a` | **极高** | **核心底座**。DataHub 总线、NetBuf。**严禁引用具体产品头文件**。 |
| **Control Plane** | `src/main_loop/` | `libedge_main.a` | 中等 | **系统决策**。Arbiter 仲裁、Scheduler 调度。严禁运行 $O(N)$ 算法。 |
| **Compute Plane** | `src/stream_engine/` | `libedge_stream.a` | 频繁 | **业务大脑**。Pipeline、JS/Lua。这是唯一允许处理业务语义的地方。 |
| **Execution Plane** | `src/io_executor/` | `libedge_io.a` | 高 | **南向四肢**。物理驱动。必须保持**“契约盲”**，仅负责搬运。 |

### 1.2 开发者避坑守则 (Red Lines)
*   **严禁向上包含**：`src/core` 下的代码绝对不准出现 `#include "contracts.h"`。
    *   *后果*：一旦包含，核心库就绑定到了具体产品（如 IEMS）上，无法再给 SG 或 EVSE 使用。
*   **严禁直接调用驱动**：算法层（Compute Plane）禁止调用 `driver_modbus_send()`。
    *   *后果*：这会导致业务逻辑与硬件强耦合。正确做法是写 DataHub 影子快照，由 Arbiter 自动分发。
*   **禁阻塞 IO**：Control 和 Compute 平面禁止调用 `sleep()`、`recv()` 等阻塞函数。
    *   *后果*：整个系统的心跳会因此停跳，导致分钟冻结丢失或 Watchdog 重启。

---

## 二、 环境治理：Python 自动化引导 (Auto-Bootstrap)

**结论先行：**
**绝对不要（Never）将 `.venv` 虚拟环境目录提交至 Git 仓库。这不仅是体积问题，更是 100% 的不可用问题。**

### 2.1 为什么 `.venv` 是工程毒药？
1.  **路径硬编码 (Hardcoded Paths)**：`bin/pip` 脚本首行会写死创建者的绝对路径（如 `/home/user/edgec/.venv/...`）。你拉取到本地后，路径不一致会导致 Python 报错 `bad interpreter`。
2.  **二进制冲突 (Binary Incompatibility)**：虚拟环境里的 `PyYAML` 等库包含编译好的 C 扩展（.so）。你在 Linux x86 编译的库，换到 ARM 边缘网关上运行会直接段错误（Segmentation Fault）。
3.  **脏产物堆积**：虚拟环境包含数千个无关的小文件，会让 `git status` 的响应时间从毫秒级退化到秒级。

### 2.2 解决方案：一键式环境对齐 SOP
新开发者进入项目后，唯一的标准动作是：
```bash
# 1. 执行一键引导脚本
bash tools/setup_env.sh
```
该脚本会自动：
*   探测系统 Python 版本。
*   创建**本地专用**的 `.venv`。
*   根据 `requirements.txt` 增量安装编译器依赖。

### 2.3 IDE 高级适配 (VSCode/CLion)
为了获得工业级的开发体验，你必须将 IDE 的 Python 解释器指向生成的虚拟环境：
*   **路径**：`${PROJECT_ROOT}/.venv/bin/python`
*   **价值**：设置正确后，你编辑 `tools/contract_compiler/` 目录下的 Python 插件时，IDE 才能正确识别 `ProtocolGenerator` 基类，并提供成员变量的智能补全。

---

## 三、 数据宪法：契约驱动开发 (CDD v2.0)

**结论先行：**
**“测点 (Tag)”是 EdgeOS 的唯一合法货币。代码中严禁出现寄存器物理地址。**

### 3.1 为什么推行 CDD？
在传统嵌入式项目中，网关开发组和从机设备组经常因为“寄存器 0x1000 到底是 A 相电压还是 B 相电压”而反复扯皮。EdgeOS 通过 **“单一事实源 (SSoT)”** 彻底解决了这个问题：YAML 契约即是真相。

### 3.2 测点（Tag）的全生命周期追踪
假设我们要接入一个新的“电池簇单体最高电压”测点：

#### 第一步：在语义层确立含义 (`contracts/schema/tags.yaml`)
在这里，我们赋予数据“灵魂”：
```yaml
- id: 4005
  name: BATT_CELL_VOLT_MAX
  type: float
  unit: "V"
  attributes:
    category: "Battery"
    ui: { group: "Cell_Status", min: 2.0, max: 4.5, precision: 3 } # 指导 UI 渲染
    northbound: { storage: "trend", push: "change" } # 指导存储策略
```

#### 第二步：在协议层绑定物理地址 (`contracts/bindings/modbus/catl_bms.yaml`)
在这里，我们定义如何“抓取”数据：
```yaml
mappings:
  - tag: BATT_CELL_VOLT_MAX
    addr: 0x4010  # 物理寄存器地址
    scale: 0.001  # 原始值 3200 -> 3.2V
```

#### 第三步：在具体产品中激活功能 (`cmd/iems/extension.yaml`)
在这里，我们进行功能裁剪：
```yaml
enabled_tags:
  - BATT_CELL_VOLT_MAX
```

#### 第四步：南向驱动自动感知 (The Magic)
在 `modbus_rtu_driver.c` 中，驱动程序只写一次：
```c
// 驱动只认物理地址，通过编译器生成的查表 API 动态转换
float scale;
edge_tag_id_t id = edge_lookup_modbus(0x4010, &scale); 
if (id != TAG_INVALID) {
    // 此时 id 自动变为 4005 (TAG_BATT_CELL_VOLT_MAX)
    // scale 自动变为 0.001
}
```
**结论：** 修改 YAML 点表定义，**SDK 核心源码 1 行都不用改**，仅需重新链接即可。

---

## 四、 逻辑内核：流水线 (Pipeline) 高级工程学

**结论先行：**
**Pipeline 是单线程、顺序执行的状态机。严禁在其中执行任何阻塞操作。**

### 4.1 理解 Pipeline 的 FSM 状态
每一条流水线在执行过程中都可能经历以下状态转换：
1.  **IDLE**：等待 DataHub 的 `uv_async` 唤醒。
2.  **RUNNING**：正在顺序执行 Stage 链。
3.  **SUSPENDED**：遇到 `STAGE_ASYNC`（如写库、耗时算法），Pipeline 挂起并保存当前事件快照。
4.  **RESUMING**：Worker 完成任务，回调 `pipeline_resume`，从中断点继续向后流转。

### 4.2 开发自定义 C 算子 (Stage) 的工业级 SOP
如果你需要开发一个复杂的 FF T 算法 Stage：

1.  **定义私有上下文** (在头文件中定义，在 `.c` 中实现):
    ```c
    typedef struct { int fft_size; float *window; } fft_stage_cfg_t; // 配置
    typedef struct { uint64_t last_calc_ts; } fft_stage_state_t;    // 运行状态
    ```
2.  **实现双重清理 (Double-Cleanup)**:
    为了达成 0 泄漏目标，必须在 `pipeline_add_stage` 时注入两个回调：
    *   `cleanup_state`: 释放计算缓存。
    *   `cleanup_cfg`: 释放从 JSON 解析出来的配置。
3.  **遵循 ASYNC 卸载原则**:
    对于 FFT 这种耗时计算，禁止在 `process` 函数中直接运行。必须利用 `uv_queue_work` 卸载到 libuv Worker 线程池，并返回 `STAGE_ASYNC`。

---

## 五、 构建系统：产品沙箱与资产保护

**结论先行：**
**`build/` 目录不是垃圾堆，而是按照“资产等级”物理隔离的产物沙箱。**

### 5.1 目录拓扑结构说明
*   **`build/sdk/lib/`**：存放 `libedge_core.a`、`libedge_main.a` 等。这些是公司的**通用资产**，全场共用，编译一次。
*   **`build/products/<name>/bin/`**：存放最终交付的二进制。
*   **`build/products/<name>/generated/`**：存放由编译器根据产品 YAML 生成的专属 `contracts.c/h`。
*   **`build/products/<name>/lib/`**：存放产品的专属点表库 `libcontract_xxx.a`。

### 5.2 为什么要把生成代码放在 `generated/` 目录下？
因为 IEMS 产品的 1001 号点叫电压，SG 产品的 1001 号点可能叫电网频率。通过物理路径隔离，我们确证了：
*   编译 IEMS 时，编译器包含路径指向 `iems/generated/`。
*   编译 SG 时，指向 `sg/generated/`。
**两者虽然文件名相同，但内涵完全不同，且互不干扰。**

---

## 六、 质量红线：LSan 与 风格定力

### 6.1 内存零泄漏 (The LSan Rule)
EdgeOS 对内存泄漏采取**“零容忍”**政策。
*   **测试要求**：每一个单元测试在执行完 `stream_engine_destroy` 后，必须配合 `while(uv_run(loop, UV_RUN_NOWAIT))` 来排空所有异步 `close_cb`。
*   **CI 门控**：只要 LeakSanitizer 报告 1 字节的泄漏，该提交将被视为非法。

### 6.2 代码格式定力 (The Handcrafted Style)
我们移除了 `pre-commit` 这种“暴力全量格式化”工具，因为它会污染第三方库。
*   **正确做法**：在提交 PR 前，**必须**运行：
    ```bash
    bash scripts/format.sh
    ```
*   **原理**：该脚本像手术刀一样，仅对 `src/`、`include/`、`cmd/` 等自有代码进行格式化，确证代码风格的统一性与纯净性。

---

> **致开发者：** 这里的每一行代码都将运行在偏远的电站或繁忙的工厂中。你的架构定力，就是 EdgeOS 的生命线。
