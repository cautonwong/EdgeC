# EdgeOS 开发者治理手册 (Industrial v2.0 - 保姆级全量版)

> **修订说明：** 本文档是 EdgeOS 的“最高开发宪法”。它不仅回答“怎么做”，更通过底层逻辑拆解告诉你“为什么”。
> 
> **面向对象：** 
> 1. **核心开发**：负责底层总线、引擎、协议栈重构。
> 2. **产品集成商**：通过配置 YAML 和编写算法快速交付产品（IEMS/HEMS/DCU）。
> 3. **从机开发者**：参考契约生成从机代码，实现网关与从机的数据对齐。

---

## 🛠️ 第一章：环境初始化 (Zero-to-Hero Setup)

**准则：环境的一致性是系统稳定性的第一道防线。**

### 1.1 系统基础环境 (Ubuntu 20.04/22.04)
不要假设你的机器什么都有，先运行以下命令补全“手术工具”：
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential cmake clang-format \
    python3-pip python3-venv \
    libsqlite3-dev libmodbus-dev \
    gcc-aarch64-linux-gnu # 如果你需要进行 ARM 交叉编译
```

### 1.2 Python 环境治理 (遵循 question.md 规范)
**⚠️ 红色禁区：严禁将 `.venv` 提交到仓库！**
*   **为什么？** 虚拟环境包含绝对路径。如果你在 `/home/user/` 创建了环境，拉到另一台机器由于路径不同，`pip` 会直接报错 `bad interpreter`。此外，Linux 下编译的二进制包（.so）在其他平台无法运行。

**正确操作流：**
1. **执行自动化引导脚本**：
   ```bash
   # 在项目根目录下执行
   bash tools/setup_env.sh
   ```
2. **检查成果**：此时根目录下会出现 `.venv` 文件夹（已被 `.gitignore` 自动忽略）。
3. **IDE 对齐 (以 VSCode 为例)**：
   *   按 `Ctrl+Shift+P` -> 输入 `Python: Select Interpreter`。
   *   选择 `Enter interpreter path...`。
   *   选择当前目录下的 `./.venv/bin/python`。
   *   **价值**：这一步能确证你编辑 `tools/contract_compiler/` 插件时，IDE 能够识别所有类型，提供工业级的代码补全。

---

## 🏗️ 第二章：架构定力 (Four Planes Architecture)

**准则：平面的边界就是生存的底线。跨平面耦合 = 架构自杀。**

EdgeOS 物理拆分为四个库，每个库都有其不可侵犯的职责边界：

### 2.1 四层逻辑平面拆解
| 平面名称 | 归属目录 | 库产物 | 开发者约束 (The Red Lines) |
| :--- | :--- | :--- | :--- |
| **Foundation (L0)** | `include/` | `*.h` | **静态宪法**。只准定义结构体原型。**绝对禁止放入业务枚举！** |
| **Foundation (L1)** | `src/core/` | `libedge_core.a` | **核心底座**。DataHub 总线、NetBuf。**严禁包含具体产品的 `contracts.h`**。核心只认 `tag_id` 整数。 |
| **Control Plane** | `src/main_loop/` | `libedge_main.a` | **决策中枢**。Arbiter 仲裁器。严禁调用驱动的 `read/write` 原始函数。 |
| **Compute Plane** | `src/stream_engine/`| `libedge_stream.a` | **业务大脑**。Pipeline FSM。**禁止做任何文件 IO 或阻塞通信**。 |
| **Execution Plane** | `src/io_executor/` | `libedge_io.a` | **物理采集**。Modbus/DLMS 驱动。必须保持**“契约盲”**，仅通过符号查表。 |

---

## 📜 第三章：契约治理体系 (Contract Governance v2.0)

**准则：所有的测点都是“法定的”。代码中出现 0x1000 这种数字是二开者的耻辱。**

### 3.1 什么是契约驱动开发 (CDD)？
它是指网关（Master）和设备（Slave）公用一份 YAML 定义。改一处 YAML，两端代码自动对齐。

### 3.2 增加一个新测点的保姆级 SOP
假设你需要为一个 IEMS 项目增加“储能柜门禁状态”。

#### 第一步：定义全局语义 (`contracts/schema/tags.yaml`)
这是为了让全公司所有产品对这个点的定义达成共识。
```yaml
- id: 5002
  name: DOOR_STATUS
  type: int
  desc: "储能柜门禁开关状态"
  attributes:
    category: "Security"
    ui: { group: "Environment", widget: "status_light" } # 指导 UI 渲染为状态灯
```

#### 第二步：绑定协议映射 (`contracts/bindings/modbus/xxx.yaml`)
这是为了让驱动知道从哪个寄存器读取。
```yaml
mappings:
  - tag: DOOR_STATUS
    addr: 0x0200  # 假设硬件在 512 寄存器上
    scale: 1.0
```

#### 第三步：在产品扩展中激活 (`cmd/iems/extension.yaml`)
```yaml
enabled_tags:
  - DOOR_STATUS
```

#### 第四步：编译并观察产物
执行 `./build_and_run.sh`。你会发现以下物理变化：
1. **生成的头文件** (`build/products/iems/generated/contracts.h`)：
   ```c
   #define TAG_DOOR_STATUS_ENABLED 1
   TAG_DOOR_STATUS = 5002,
   ```
2. **生成的查表代码** (`build/products/iems/generated/contracts.c`)：
   ```c
   static const modbus_map_t s_modbus_map[] = {
       { 0x0200, TAG_DOOR_STATUS, 1.0f },
       ...
   };
   ```

---

## 🧠 第四章：计算平面实战 (Pipeline & Stages)

**准则：Pipeline 是顺序执行的。Stage 内部的内存必须“双向闭环”。**

### 4.1 理解“双重清理”机制 (Double-Cleanup)
为了达成工业级的内存零泄漏，每一个动态加载的 Stage 节点都持有两类数据：
1. **`priv_cfg`**: 来自 JSON 配置的静态参数（如阈值、数据库路径）。
2. **`priv_state`**: 运行时产生的中间状态（如累加器、滑动平均缓存）。

**开发者守则：** 如果你在 Stage 工厂里 `malloc` 了内存，你必须在 `pipeline_add_stage` 时注入对应的 `cleanup_cfg` 或 `cleanup_state` 钩子。

### 4.2 开发一个自定义 C 算子示例
```c
// 1. 定义配置
typedef struct { double max_temp; } my_cfg_t;

// 2. 处理逻辑
stage_ret_t my_temp_guard_proc(pipeline_stage_t *s, pipe_event_t *ev) {
    my_cfg_t *cfg = (my_cfg_t*)s->priv_cfg;
    if (ev->payload.val > cfg->max_temp) {
        LOG_FATAL(LOG_CAT_CORE, "Temp too high! System Emergency Stop!");
        return STAGE_DROP; // 拦截后续持久化或上送逻辑
    }
    return STAGE_OK;
}

// 3. 注入时确证清理
pipeline_add_stage(p, "guard", my_temp_guard_proc, 
                   NULL, NULL,           // 无运行状态
                   cfg, free);           // 注入配置，并委托引擎使用 free 释放
```

---

## 🌐 第五章：北向与可观测性

### 5.1 影子读取接口 (Shadow Read API)
Web UI 展示实时数据时，禁止轮询 DataHub 的共享内存。
*   **接口**：`GET /api/v1/data/latest/{id}`
*   **原理**：它读取的是 DataHub 的影子缓存（Shadow Cache），这通过 `uv_rwlock_t` 实现了读写隔离，确证了 UI 查询绝不干扰实时采集的确定性。

### 5.2 契约下发接口
前端 HMI 开发者必须调用：
*   **接口**：`GET /api/v1/contracts`
*   **原理**：它返回合并后的 `contracts.yaml`。前端应根据此文件动态渲染测点名称、量程和分组，实现“一份代码适配百种产品”的动态 UI 定力。

---

## 🚀 第六章：构建产物资产管理 (Artifacts)

**准则：build 目录不是垃圾堆，而是严格隔离的沙箱。**

编译完成后，请通过以下路径寻找你的产物：

1. **`build/sdk/lib/`**:
   *   `libedge_core.a`: 核心库。**请保护此文件**，它是所有平面的基石。
   *   `libedge_main_loop.a`: 控制面库。
2. **`build/products/<name>/bin/`**:
   *   这里是最终交付的二进制文件（如 `iems`）。
3. **`build/products/<name>/generated/`**:
   *   这里是当前产品专属的查表代码。**调试时请优先查看 contracts.yaml 确认点表合并是否正确。**
4. **`build/tests/`**:
   *   存放所有的单元测试程序。

---

## 🛡️ 第七章：质量与提交红线 (The CI Gates)

在执行 `git commit` 前，请务必进行自检：

1. **精准格式化**：
   运行 `bash scripts/format.sh`。不要试图手动调整缩进，让工具说话。
2. **内存审计**：
   运行 `./build_and_run.sh`。如果在控制台看到 `AddressSanitizer: 1 byte(s) leaked`，说明你的 Stage 销毁钩子没写对。**严禁带泄漏提交！**
3. **解耦性检查**：
   搜索你的代码，确认没有在 `src/core` 下包含任何具体产品的头文件。

---

> **结语：** EdgeOS 的架构定力来源于每一位开发者的克制。请记住，你写的不仅仅是 C 代码，而是未来 15 年工业现场的确定性保障。