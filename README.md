# EdgeOS (EdgeC) - 工业级边缘计算核心运行时 SDK

EdgeOS 是一个专为 **工业物联网 (IIoT)**、**能源管理系统 (EMS)** 和 **智能电网融合终端** 打造的高性能、确定性、事件驱动型边缘计算 SDK。它采用 C11 标准构建，核心基于 `libuv` 事件循环，旨在 15 年以上的超长生命周期内提供稳如磐石的服务。

---

## 🏗️ 1. 核心架构设计 (The "Four Planes" Architecture)

遵循 **core_v2.md** 规范，系统划分为四个物理隔离的逻辑平面，通过严密的契约层进行解耦。

### 1.1 系统平面分布
| 平面 | 库产物 | 职责描述 |
| :--- | :--- | :--- |
| **Foundation (L0)** | `contract_data_types.h` | **数据宪法**。定义元数据与协议映射的静态 Schema，不依赖任何库。 |
| **Foundation (L1)** | `libedge_core.a` | **核心底座**。包含 DataHub (总线)、NetBuf (内存池)、Params (参数中心)。 |
| **Control Plane** | `libedge_main_loop.a` | **决策中枢**。Arbiter (仲裁器)、Scheduler (调度器)、REST Server。 |
| **Compute Plane** | `libedge_stream_engine.a` | **计算引擎**。Stream Engine、Pipeline 调度、有状态 JS/Lua 算法。 |
| **Execution Plane** | `libedge_io_executor.a` | **南向执行**。IO Executor、契约化协议驱动 (Modbus/DLMS/CAN)。 |

---

## 📜 2. 契约治理体系 (Contract Governance v2.0)

EdgeOS 采用“契约驱动开发 (CDD)”模式，通过 YAML 定义系统的“数据宪法”，并自动生成高性能的 C 语言查表代码。

### 2.1 契约三级模型
| 层级 | 文件路径 | 职责描述 |
| :--- | :--- | :--- |
| **L1: 全局语义** | `contracts/schema/tags.yaml` | **定义“是什么”**。规定 TagID、名称、单位、类型及 UI 展现元数据。 |
| **L2: 协议绑定** | `contracts/bindings/` | **定义“怎么拿”**。规定特定设备型号（如 Acrel_P80）的寄存器映射。 |
| **L3: 产品定制** | `cmd/<prod>/extension.yaml` | **定义“我要谁”**。选择开启哪些功能，或定义项目现场私有测点。 |

### 2.2 实战指南：新增一个“机柜温度”测点

#### 第一步：在全局字典中定义语义 (`tags.yaml`)
```yaml
- id: 1006
  name: CABINET_TEMP
  type: float
  unit: "C"
  attributes:
    ui: { group: "Environment", min: -20, max: 80, precision: 1 }
    northbound: { storage: "trend" }
```

#### 第二步：在 Modbus 点表中绑定物理地址 (`bindings/modbus/xxx.yaml`)
```yaml
mappings:
  - tag: CABINET_TEMP
    addr: 0x0100  # 对应硬件 256 寄存器
    scale: 0.1    # 原始值 255 -> 25.5 C
```

#### 第三步：在产品中启用该测点 (`extension.yaml`)
```yaml
enabled_tags:
  - CABINET_TEMP
```

#### 第四步：在 C 语言驱动中引用 (自动感知)
```c
// 驱动层无需修改代码，编译后自动支持：
float scale;
edge_tag_id_t id = edge_lookup_modbus(0x0100, &scale); 
// id 会自动变为 1006 (TAG_CABINET_TEMP)
```

---

## 🛠️ 3. 开发与构建

### 3.1 目录规范
*   `src/core/`: 跨 Loop 核心组件。
*   `src/main_loop/`: 控制平面实现。
*   `src/stream_engine/`: 计算平面实现。
*   `src/io_executor/`: 执行平面实现。
*   `contracts/`: 全局契约定义文件。

### 3.2 构建专业布局
构建系统（CMake）会自动将产物组织为工业级布局：
```text
build/
├── sdk/lib/               # 通用 SDK 静态库
├── products/              # 产品隔离沙箱 (bin, lib, generated)
└── tests/                 # 统一测试套件
```

### 3.3 快速编译与测试
```bash
./build_and_run.sh  # 自动化生成契约代码、编译、静态检查并运行 19 个单元测试
```

---

## 📅 4. 路线图与待办事项 (TODO)

### 🚀 核心功能
- [x] **契约治理 v2.0**：实现类型与数据的物理隔离。
- [x] **分层解耦架构**：完成四个逻辑平面的库拆分。
- [x] **契约化驱动**：Modbus 驱动实现 0 硬编码转换。
- [ ] **Lua 有状态算法引擎**：集成 Lua 运行时支持闭包状态计算。

### 🌐 北向交互
- [x] **元数据下发接口**：`/api/v1/contracts` 接口已实装。
- [ ] **SQLite 历史点查 API**：完善曲线与账本查询。
- [ ] **Web UI 门户**：基于 Vue/React 实现动态元数据驱动的 HMI。

### 🛡️ 质量硬化
- [x] **100% 测试覆盖**：19 个核心测试用例全部 PASS。
- [ ] **抗跳变极限压测**：模拟 NTP 剧烈跳变下的冻结一致性。
- [ ] **全链路零拷贝**：跨线程数据交换全面切换为 `net_buf_t`。

---
