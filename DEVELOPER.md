# EdgeOS 开发者治理指南 (Industrial v2.0 - Frozen Edition)

> **前言：** EdgeOS 不是一个简单的 C 语言库，而是一套面向 **15 年超长生命周期**、具备**跨端一致性**的工业级治理体系。
>
> 这里的代码必须具备“物理级的确定性”。为了达成这一目标，本指南定义的每一条规范都是通过血泪教训沉淀下来的**工程定力**。

---

## 一、 架构定力：四层逻辑平面的“物理隔离”

**结论先行：**
**在 EdgeOS 中，平面的边界即是安全的红线。严禁任何跨平面的逻辑耦合。**

我们为什么要把系统拆成四个平面？因为工业现场的需求极其多变，但底层内核必须稳如磐石。

### 1.1 平面分工与 ABI 稳定性
| 平面名称 | 物理归属 | 核心职能 | 稳定性要求 | 开发者约束 (Red Lines) |
| :--- | :--- | :--- | :--- | :--- |
| **Foundation (L0)** | `include/` | **数据宪法**。定义全系统通用的 Struct 和 Enum。 | **永不改变** | 严禁放入任何逻辑代码或静态变量。 |
| **Foundation (L1)** | `src/core/` | **核心底座**。DataHub 总线、NetBuf 内存池、参数持久化。 | **极高** (ABI 级) | **严禁包含具体产品的 `contracts.h`**。Core 只认 `tag_id` 整数。 |
| **Control Plane** | `src/main_loop/` | **系统决策**。Arbiter 仲裁、Scheduler 调度、REST API。 | 中等 | 严禁执行任何阻塞 IO 或耗时算法，决策必须在 1ms 内完成。 |
| **Compute Plane** | `src/stream_engine/`| **业务大脑**。Pipeline FSM、JS/Lua 策略、数据聚合。 | 频繁变动 | 严禁直接调用驱动函数。数据交互必须通过 DataHub 影子流。 |
| **Execution Plane** | `src/io_executor/` | **南向执行**。各种物理驱动 (Modbus/CAN/DLMS)。 | 高 (协议级) | **必须保持“契约盲”**。驱动不准知道 1001 是电压，只需查表搬运。 |

---

## 二、 环境管理：自动化引导 (Auto-Bootstrap)

**结论先行：**
**绝对不要（Never）将 `.venv` 虚拟环境提交至 Git 仓库。**

### 2.1 为什么 `.venv` 是工程毒药？
1.  **绝对路径硬编码**：`pip` 内部会写死创建者的绝对路径，换台机器直接报 `bad interpreter`。
2.  **二进制不兼容**：你在 Linux x86 编译的 `.so` 库，在 ARM 架构的边缘网关上完全无法运行。
3.  **仓库污染**：数以千计的第三方小文件会拖慢 `git status` 的速度。

### 2.2 最佳实践：一键式环境对齐
新开发者进入项目后，唯一的正确动作是：
```bash
# 执行自动化引导脚本
bash tools/setup_env.sh
```
这个脚本会根据 `requirements.txt` 自动为你创建本地隔离的 Python 环境。
*   **IDE 适配**：如果你使用 VSCode，请务必将 `.venv/bin/python` 设置为默认 Interpreter。这能确证你编辑 YAML 插件时，IDE 能够提供完整的语法检查。

---

## 三、 数据宪法：契约驱动开发 (CDD v2.0)

**结论先行：**
**“测点 (Tag)”是 EdgeOS 全局唯一的通用货币。严禁在代码中出现寄存器地址。**

### 3.1 为什么推行 CDD？
在传统模式中，网关开发和设备开发（从机）经常因为点表对不上而“打架”。EdgeOS 通过 **“一源双向生成”** 解决了这个问题：通过一份 YAML 契约，自动为两端生成一致的 C 代码。

### 3.2 实战 SOP：新增一个“逆变器内壳温度”测点
假设你正在为一个新项目接入华为逆变器：

#### Step 1: 语义层定义 (`contracts/schema/tags.yaml`)
这是定义“它是什么”。
```yaml
- id: 1007
  name: INV_SHELL_TEMP
  type: float
  unit: "C"
  attributes:
    category: "Metrology"
    ui: { group: "Safety", min: 0, max: 120, precision: 1 } # 指导 UI 自动生成仪表
    northbound: { storage: "trend" } # 指导系统自动存库
```

#### Step 2: 协议层绑定 (`contracts/bindings/modbus/huawei_v3.yaml`)
这是定义“怎么拿到它”。
```yaml
mappings:
  - tag: INV_SHELL_TEMP
    addr: 0x3005  # 寄存器物理地址
    scale: 0.1    # 缩放系数
```

#### Step 3: 产品层激活 (`cmd/iems/extension.yaml`)
这是定义“哪些产品需要它”。
```yaml
enabled_tags:
  - INV_SHELL_TEMP
```

#### Step 4: 驱动感知 (无需改代码)
驱动层代码早已通过 `extern` 引用了 `edge_lookup_modbus`。当你完成 YAML 修改并编译时：
1.  编译器自动生成新的查表逻辑。
2.  驱动读到 `0x3005` 时，会瞬时将其映射为 ID `1007`。
3.  **业务逻辑（Pipeline）直接订阅 `TAG_INV_SHELL_TEMP` 即可拿到真实温度。**

---

## 四、 逻辑内核：流水线 (Pipeline) 实战

**结论先行：**
**Pipeline 是单线程顺序执行的。严禁在 Pipeline 中写死循环。**

### 4.1 开发自定义 C 算子 (Stage)
如果你需要实现一个行业特有的高频算法（如 FFT），请遵循以下模板：

1.  **定义私有配置 (Context)**:
    ```c
    typedef struct { double threshold; } my_stage_cfg_t;
    ```
2.  **实现处理函数**:
    ```c
    stage_ret_t my_algorithm_proc(pipeline_stage_t *stage, pipe_event_t *ev) {
        my_stage_cfg_t *cfg = (my_stage_cfg_t*)stage->priv_cfg;
        // 逻辑...
        return STAGE_OK; // 或 STAGE_DROP 拦截
    }
    ```
3.  **实现双重清理 (Double-Cleanup)**:
    为了保证内存零泄漏，必须提供 `cleanup_state` 和 `cleanup_cfg` 两个回调。

---

## 五、 构建、产物与质量红线

### 5.1 专业级产物沙箱 (The Build Sandbox)
EdgeOS 的构建产物是高度组织化的，禁止在 `build/` 下乱堆放：
*   **`build/sdk/lib/`**: SDK 四大逻辑平面的通用库。它们是编译一次，全产品共享的资产。
*   **`build/products/<name>/bin/`**: 最终的二进制程序物理归口。
*   **`build/products/<name>/generated/`**: 每一个产品独有的“数据宪法”实现。

### 5.2 质量三红线
1.  **LSan 零容忍**：任何 1 字节的内存泄漏，即使测试用例能过，CI 也会将其判为失败。
2.  **风格精准化**：提交前必须运行 `./scripts/format.sh`。它只会格式化项目源码，不会破坏第三方库的原始面貌。
3.  **解耦物理化**：严禁在 `src/core` 下包含任何产品的头文件。如果你的 Core 代码需要知道“IEMS 包含哪些点”，那说明你的设计已经失败了。

---

> **结语：** 每一个成熟的开发者都应该是架构的守护者。在 EdgeOS 中，我们守护的是**契约的一致性**和**分层的纯净性**。