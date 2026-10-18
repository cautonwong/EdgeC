# 架构白皮书：去全局契约与标签体系演进 (No-TagID, No-Contract, No-Contract-Compiler)

> **文档版本**: 1.0.0  
> **状态**: 批准实施 (Approved)  
> **有界上下文**: EdgeC / 全局架构治理  
> **核心原则**: 消费者定接口 (Consumer-Driven Contracts)、DDD 聚合根自治、依赖倒置 (DIP)、防腐层 (ACL)

---

## 1. 架构演进背景与问题定义

### 1.1 历史背景：契约中心主义的诞生与异化
在 EdgeC 早期演进（AEP-301 / AEP-302 / ADR-0002）中，系统为了统一多协议测点配置，引入了**全局契约编译器体系**：
- 在 `contracts/schema/` 和 `contracts/bindings/` 维护大量的 YAML 定义；
- 通过 Go 编写的 `contract-compiler` 离线/编译期解析 YAML，在各产品目录的 `generated/` 下生成庞大的 `contracts.h`、`contracts.c`、`edge_driver_registry.c`、`tags.go`、`tags_type.ts`；
- 分配全局唯一的连续数字 `TagID`（如 1001 代表 A相电压，1002 代表 A相电流，3001 代表累计有功电能）。

### 1.2 现实矛盾：全局中心化与领域自治的冲突
随着系统向微内核、DDD 聚合根自治与真正的工业场景演进，这一套体系暴露出严重的架构反模式与工程阻碍：

```
[历史反模式：上帝契约穿透]
YAML 集中声明 ──> contract-compiler ──> 全局 contracts.h (数千个魔数宏与哈希表)
                                              │
         ┌────────────────────────────────────┼────────────────────────────────────┐
         ▼                                    ▼                                    ▼
   南向驱动层 (DLT698/645)              产品聚合根 (DCU/IEMS)                北向通道与数据底座
强行把 698 OAD 翻成 TagID             代码硬编码 TAG_ 宏，破坏自治           被全局连续数字绑死
```

1. **违背“消费者定接口”与依赖倒置（DIP）**：
   - 核心框架与具体业务被全局 YAML 绑架。修改一个产品的测点或协议，导致所有产品的构建脚本被触发，重新生成全局头文件；
   - 破坏了有界上下文：储能产品（IEMS）与电力集中器（DCU）的业务领域、量纲与寻址规则完全不同，却被强行放在同一个全局命名空间与数字池中。
2. **严重的“二次翻译”运行时开销与语义扭曲**：
   - 以电力集中器（DCU）为例：电表设备遵循 DL/T 698.45（原生使用 32-bit OAD，如 `0x20000200` 代表 A相电压），电力主站也遵循 698 规约；
   - 但因为全局契约的存在，南向驱动解析出 `0x20000200` 后，必须查编译器生成的哈希表翻译成 `1001`（`TAG_VOLTAGE_PHASE_A`），塞进消息总线；
   - DCU 的业务调度器（Poller/Router）读出 `1001` 后，为了满足国家电网业务与存储要求，又不得不写大量 `if-else` 把 `1001` 翻回 `0x20000200`；
   - 这种“南向翻译一次、北向翻译一次”的循环完全是虚构的架构内耗。
3. **构建系统重型化与工具链脆弱性**：
   - 编译一个轻量的 C 应用，必须依赖 Go 语言运行时执行 `contract-compiler`，跨平台交叉编译与嵌入式构建成本高昂。

---

## 2. 源码事实审计与解剖 (Codebase Reality Audit)

经过对 EdgeC 仓库全链路代码的严格排查，我们获得了一系列确凿的技术事实：

### 2.1 基础模块层 (`modules/`) 的真实纯洁度
整个 `modules/` 目录（基础框架、核心总线、存储、驱动）**没有任何一个文件包含了 `#include "contracts.h"`**：
- **核心数据总线 (`modules/edge_core/datahub`)**：
  - 查看 `datahub.h`，DataHub 内部只认一个标量 `uint32_t tag_id` 和一个应用层注入的回调指针：
    ```c
    typedef int (*datahub_tag_map_cb_v2)(uint32_t tag_id, uint32_t device_id);
    ```
  - **DataHub 源码压根没有 `contracts.h`，它根本不在乎这个数字是全局 TagID、还是电力的 698 OAD，亦或是本地 Channel ID**！槽位寻址完全由应用注入的映射函数决定。
- **南向驱动框架 (`modules/edge_io/edge_driver`)**：
  - `edge_driver_ctx_t` 采用标准的依赖注入接口：
    ```c
    const void *(*lookup_by_key)(uint64_t key);
    const void *(*lookup_by_tag)(uint32_t tag_id);
    ```
  - 驱动模板自身不认识 `contracts.h`，只向挂载它的组合根/适配器索要映射函数指针。
- **南向协议栈 (`modules/edge_protocol/`)**：
  - `modbus_codec.c`、`dlt645_codec.c` 等编解码器为纯 C 算法，100% 零依赖，与契约编译器无任何关联。

### 2.2 DCU 业务层的原生领域事实
- 查看 `cmd/dcu/dcu_storage.c`，底层的 SQLite 表结构原生定义为：
  ```sql
  CREATE TABLE IF NOT EXISTS dcu_telemetry_latest (
      meter_address TEXT, 
      oad INTEGER,          -- 字段名直接就是 DL/T 698 oad！
      value REAL, 
      updated_at INTEGER, 
      PRIMARY KEY (meter_address, oad)
  );
  ```
- `cmd/dcu/dcu_storage.c` 的存储写入接口签名直接是：
  ```c
  int dcu_storage_write_telemetry(..., uint32_t oad, double value);
  ```
- `cmd/dcu/dcu_node.c` 中虽然写了 `#include "contracts.h"`，但在整个 242 行源码中，**没有使用任何一个来自 `contracts.h` 的宏定义**，纯属死代码！

### 2.3 外部工程与下游依赖排查
- **Web 前端 (`web/`)**：消费的是北向通过 WebSocket/REST 返回的 JSON 指标名。
- **时序数据采集器 (`twin-data-collector` / TDengine)**：消费的是 MQTT 报文中的 JSON 测点键名（如 `"Ua": 220.5`）。
- **结论**：没有任何外部系统把 C 头文件 `contracts.h` 作为 ABI。外部系统依赖的是**北向序列化协议（JSON/MQTT）**，底层 C 语言领域模型完全可以自决内部标识。

---

## 3. 终态架构蓝图：去中心化聚合根自治

### 3.1 核心设计理念

```
                           ┌─────────────────────────────┐
                           │   北向网络 / 外部消费端      │
                           │   (MQTT / REST / Web UI)    │
                           └──────────────▲──────────────┘
                                          │ 统一标准 JSON / 领域语义
                           ┌──────────────┴──────────────┐
                           │      北向协议适配层 (ACL)    │
                           └──────────────▲──────────────┘
                                          │ 领域事件 / 领域方法调用
┌─────────────────────────────────────────┴─────────────────────────────────────────┐
│ 产品专属聚合根 / 编排器 (cmd/<product>/)                                             │
│                                                                                   │
│   ┌────────────────────────────────┐       ┌──────────────────────────────────┐   │
│   │   领域模型与状态机               │       │   本地测点/通道寻址 (Local Map)    │   │
│   │   (DCU: 698 OAD / DI)          │ ◄───► │   (O(1) Jump Table / Static Dict)│   │
│   │   (IEMS: Register / BMS Cell)  │       │   (彻底消灭全局编译器生成的哈希表) │   │
│   └────────────────▲───────────────┘       └──────────────────┬───────────────┘   │
│                    │ 强类型结构体交付                         │ 槽位索引          │
│   ┌────────────────┴───────────────┐       ┌──────────────────▼───────────────┐   │
│   │   南向防腐适配层 (ACL)          │       │   DataHub 影子存储与 IPC 通道    │   │
│   │   (Adapter / Translation)      │       │   (Outbound Cache / SHM Ring)    │   │
│   └────────────────▲───────────────┘       └──────────────────────────────────┘   │
└────────────────────┼──────────────────────────────────────────────────────────────┘
                     │ 原始报文 / 帧驱动
┌────────────────────┴──────────────────────────────────────────────────────────────┐
│ 南向工业协议栈与驱动 (modules/edge_protocol & edge_io)                                │
│ (Modbus RTU/TCP, DL/T 645, DL/T 698.45, CAN, IEC 104, 零外部业务依赖)             │
└───────────────────────────────────────────────────────────────────────────────────┘
```

1. **聚合根自治 (Aggregate Root Autonomy)**：
   - 彻底删除全局 `contracts/`、`contract-compiler` 与 `contracts.h`；
   - 每个产品在 `cmd/<product>/` 维护专属聚合根与组合根（如 `dcu_node_t`, `iems_node_t`）；
   - 产品内部测点使用其行业天然标识（如电力 DCU 使用 698 OAD，通用工业使用从机地址+寄存器偏移），无需全局统一数字 ID。
2. **适配器防腐层 (Anti-Corruption Layer, ACL)**：
   - 协议适配器解析出的数据，组装为产品领域强类型结构体或以原生 OAD/DI 交付；
   - 协议驱动的点表映射（`lookup_by_key` / `lookup_by_tag`）由各产品内部静态声明并依赖注入，不再由全局跨产品生成。
3. **编排器协同与事件驱动 (Domain Events & EDA)**：
   - `cmd/` 下编排代码负责协调多个聚合根，并驱动领域方法；
   - 状态变更产生**领域事件**，编排器可发布到本地事件队列或跨进程总线，天然支持未来向独立微服务演进。
4. **DataHub 定位归位 (DataHub as Outbound Channel)**：
   - DataHub 降级为聚合根背后的**高性能发布与影子存储通道**；
   - 提供 POSIX SHM Ring 零拷贝跨进程 IPC、SeqLock 无锁高频读与 DDS 网格同步，不再充当全局上帝测点字典。

---

## 4. DCU 单点 PoC (概念验证) 实施细则

鉴于 DCU 已经拥有成熟的 `dcu_node_t` 聚合根雏形，且领域模型与 DL/T 698 OAD 天然契合，率先在 DCU 实施单点概念验证。

### 4.1 实施改造点矩阵

| 文件路径 | 现存问题 | 改造方案 |
| :--- | :--- | :--- |
| `cmd/dcu/CMakeLists.txt` | 调用 `edge_add_contract` 并强制依赖 `contract_dcu` | **彻底移除** `edge_add_contract` 与 `contract_dcu` 依赖，解绑 Go 编译器。 |
| `cmd/dcu/dcu_node.c` | 包含无用的 `#include "contracts.h"` | 直接删除该引用。 |
| `cmd/dcu/dcu_router.h` | 包含 `#include "contracts.h"` | 删除该引用，直接使用已有的 698 标准 OAD 宏。 |
| `cmd/dcu/dcu_router.c` | 通过 `edge_lookup_dlt698` 把 OAD 翻成 1001，再通过 1001 映射槽位 | 改造 `dcu_tag_map_fn` 为直接按 OAD 分发，性能提升为 O(1)。 |
| `cmd/dcu/dcu_driver_adapter.c` (新增) | 驱动点表此前依赖编译器生成的全局注册表 | 本地声明静态常量点表并提供 `dcu_driver_registry_init` 与注入回调。 |
| `cmd/dcu/main.c` | 调用 `contract_register` 与 `contract_model_find` | 改为调用本地 `dcu_driver_adapter` 接口，解耦全局单例。 |
| `tests/CMakeLists.txt` | `test_dcu_power_quality` 等误连 `contract_dcu` | 清除无效链接，确保测试全绿。 |

### 4.2 映射函数对比

**改造前（冗余哈希查找）**：
```c
int dcu_tag_map_fn(uint32_t tag_id, uint32_t device_id) {
    if (tag_id >= 0x00010000) {
        float scale = 1.0f;
        edge_tag_id_t resolved = edge_lookup_dlt698((uint64_t)tag_id, &scale);
        if (resolved != 0) tag_id = (uint32_t)resolved;
    }
    if (tag_id == TAG_VOLTAGE_PHASE_A || tag_id == 1001) offset = 0;
    ...
}
```

**改造后（原生 O(1) 跳转）**：
```c
int dcu_tag_map_fn(uint32_t oad, uint32_t device_id) {
    uint32_t offset = 0;
    switch (oad) {
        case OAD_VOLTAGE:       // 0x20000200 A相电压
        case 1001:              // 兼容旧 mock 标量
            offset = 0; break;
        case OAD_ACTIVE_ENERGY: // 0x00100200 累计有功电能
        case 3001:
            offset = 1; break;
        case OAD_CURRENT:       // 0x20010200 A相电流
        case 1002:
            offset = 2; break;
        default:
            return -1;
    }
    return (int)(device_id * 10 + offset);
}
```

---

## 5. 验收标准与推进行动项

1. **编译期独立性**：`ninja dcu` 编译时不再生成任何 `build/products/dcu/generated/` 契约文件，彻底脱离 `contract-compiler`。
2. **测试全回归**：运行 `ctest -R "test_dcu_"`，所有 14 个 DCU 单元与集成测试用例 100% 绿灯。
3. **数字孪生闭环**：运行 `dcu` 主程序，三轮马拉松测试正常完成，SQLite 数据库正常生成并完整落盘。
