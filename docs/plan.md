好，我们现在**不再写代码**，而是把之前所有讨论的 **feature matrix + 架构原则 + 已完成组件**，收敛成一份**可以直接用来拉项目、拆人力、排期、验收的实施计划**。

我会按 **“从不可变内核 → 可复用基础设施 → 产品差异层”** 来制定，这样五个产品不会失控。

---

# EdgeOS SDK 实施计划（Detailed Execution Plan）

> 目标：
> **一套 SDK，支撑 HEMS / IEMS / SG-EL2025 / EVSE / DCU 五产品**
> **所有高频路径可预测、可测试、可裁剪**

---

## 一、总原则（作为所有阶段的约束）

### 三条铁律（已经在前面反复验证过）

1. **libuv-first**

   * I/O、Timer、Thread、IPC 一律优先 libuv
   * 只有 HAL 层允许原生 syscall

2. **Data ≠ Control**

   * Control Plane：对象 / 回调 / 生命周期清晰
   * Data Plane：零拷贝 / shm_ring / ref 传递

3. **先做 IEMS 级别，再裁剪**

   * 以最复杂的 IEMS / SG-EL2025 为基准
   * HEMS / EVSE / DCU 通过关闭模块降级

---

## 二、总体实施阶段划分（6 个阶段）

```
Phase 0: 工程化骨架
Phase 1: Core Runtime（不可变内核）
Phase 2: Data Plane（DataHub / shm）
Phase 3: IO & Bus Plane（Arbiter / Executor）
Phase 4: Service Plane（Storage / Algorithm / Security）
Phase 5: Product Assembly（五产品裁剪）
```

下面逐一展开。

---

## Phase 0 —— 工程化骨架（1~2 周）

### 目标

> **没有这个阶段，后面全是技术债**

### 交付物

#### 0.1 目录结构（你已经定义，落实）

```
edge-os-sdk/
├── cmake/
├── vendor/
├── .local/              # 预编译 libuv/sqlite/cmocka
├── include/
├── src/
│   ├── core/
│   ├── data/
│   ├── io/
│   ├── services/
│   ├── security/
│   └── apps/
├── tests/
├── tools/
└── .github/workflows/
```

#### 0.2 预编译依赖（双架构）

| 依赖           | x86_64 | arm64 |
| ------------ | ------ | ----- |
| libuv        | ✔      | ✔     |
| sqlite (WAL) | ✔      | ✔     |
| cmocka       | ✔      | ✔     |
| openssl      | ✔      | ✔     |

安装到：

```
.local/linux-x86_64/
.local/linux-arm64/
```

#### 0.3 工具链与 CI

* CMake Presets

  * `linux-debug` (ASan / UBSan)
  * `linux-release`
  * `arm64-release`
* GitHub Actions

  * build + test
  * cmocka 必须全绿

✔ **验收标准**：
一个最小 main + cmocka 能在 x86 / arm 同时跑通

---

## Phase 1 —— Core Runtime（SDK 的“内核”）（2~3 周）

> **这一层：五个产品 100% 共享，不允许分叉**

### 1.1 EdgeContext（系统神经中枢）

**你已经定义，补全生命周期**

* init / start / stop
* 所有模块句柄集中
* 严禁全局变量

📦 文件：

```
core/edge_context.c
tests/test_edge_context.c
```

---

### 1.2 内存系统（双轨制）

#### 控制面

* net_buf / slab
* refcount
* 小对象稳定

#### 数据面

* dma-buf / shm heap（抽象接口）

📦 文件：

```
core/mem_ctrl.c
core/mem_data.c
tests/test_mem_ctrl.c
```

✔ **验收**：

* ASan 下无泄漏
* ref 正确归零

---

### 1.3 Scheduler v4（TimerFD + 抗跳变）

你之前方案已经很成熟，这里是**落地与测试**

📦 文件：

```
core/scheduler.c
tests/test_scheduler_jump.c
```

✔ 验收：

* 修改系统时间不乱
* cancel O(1)

---

## Phase 2 —— Data Plane（数据治理核心）（2 周）

> **这是你问 shm_ring / kv / 500 表聚合的根**

### 2.1 shm_ring（已完成 ✔）

* seqlock
* 固定 slot
* out-of-band ref

📦（已完成）

```
data/shm_ring.c
tests/test_shm_ring.c
```

---

### 2.2 DataHub（= shm_ring + notify）

**关键点：**

* 不关心谁消费
* 写完必唤醒
* 不阻塞 producer

#### v1：线程内（uv_async）

#### v2：进程间（uv_pipe）

📦 文件：

```
data/datahub.c
tests/test_datahub_thread.c
tests/test_datahub_pipe.c
```

✔ 验收：

* 无 busy loop
* 多 consumer 不丢事件

---

## Phase 3 —— IO & Bus Plane（最复杂）（3~4 周）

> **RS485 / HPLC / CAN / 透传 / OTA 全在这**

### 3.1 IO Executor（已完成 ✔）

* uv_thread
* 阻塞 IO
* 不接触业务

📦

```
io/io_executor.c
tests/test_io_executor.c
```

---

### 3.2 Arbiter v4（核心竞争点）

**能力矩阵：**

| 能力  | IEMS | SG-EL2025 |
| --- | ---- | --------- |
| 多总线 | ✔    | ✔         |
| 独占  | ✔    | ✔         |
| 抢占  | ✔    | ✔         |
| 透传  | ✔    | ✔         |
| OTA | ✔    | ✔         |

**关键设计已定：**

* Arbiter = 逻辑（可在 uv_loop）
* Executor = 手脚（uv_thread）
* Arbiter 本身不是线程（你后面纠正得非常对）

📦

```
io/arbiter.c
tests/test_arbiter_preempt.c
tests/test_arbiter_exclusive.c
```

✔ 验收：

* 周期抄表 + 透传不冲突
* OTA 全程独占

---

## Phase 4 —— Service Plane（可裁剪）（3 周）

### 4.1 Algorithm Service（你反复提到）

**典型：500+ 电表聚合**

* 独立 uv_loop
* DataHub 订阅
* Window / Snapshot / Accumulate

📦

```
services/algorithm.c
tests/test_algorithm_sum.c
```

✔ 验收：

* 高并发不阻塞 IO
* 可并行多个算法

---

### 4.2 Storage Service（SQLite WAL）

**明确回答你之前的问题：**

> ❗ SQLite **必须单独线程**

* 写：uv_queue_work（WAL）
* 读：

  * 高频参数：cache
  * 冻结数据：同步 read（短）

📦

```
services/storage.c
tests/test_storage_wal.c
```

---

### 4.3 Security Service（按产品开关）

* SE Driver
* OpenSSL Engine

📦

```
security/se_driver.c
security/openssl_engine.c
```

---

## Phase 5 —— Product Assembly（2 周）

### 5.1 Feature Matrix → 编译裁剪

| 产品        | Bus      | Arbiter | DataHub | Algo | App       |
| --------- | -------- | ------- | ------- | ---- | --------- |
| HEMS      | MQTT     | ❌       | ✔       | ❌    | Native    |
| IEMS      | 485/HPLC | ✔       | ✔       | ✔    | Native    |
| SG-EL2025 | 全        | ✔       | ✔       | ✔    | Container |
| EVSE      | 单485     | 简化      | ✔       | ❌    | Native    |
| DCU       | 混合       | ✔       | ✔       | 可选   | Native    |

### 5.2 配置驱动

* JSON / YAML
* CMake Preset
* 无 ifdef 灾难

📦

```
apps/iems/
apps/sg_el2025/
```

---

## 最终里程碑（你可以直接用来立项）

| 里程碑 | 标志                        |
| --- | ------------------------- |
| M1  | Core Runtime 全测试          |
| M2  | DataHub + Arbiter 跑真实 485 |
| M3  | IEMS 500 表稳定              |
| M4  | OTA + 透传不影响采集             |
| M5  | 五产品裁剪完成                   |

---
