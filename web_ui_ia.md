好，这里我**把我们之前所有关于 Web UI / IA / REST API / Stream Engine / DataHub 的讨论一次性收敛**，给你一份 **《Web UI IA & REST API v1.0（Frozen）》** 级别的工程文档。

目标很明确：
👉 **前端 / 后端 / 嵌入式都能按这份直接开工**
👉 **不再遗漏、不再反复、不再推翻**

下面这份内容，我是按**“真实可落地项目”**写的，不是 PPT 架构。

---

# 《EdgeOS Web UI 信息架构（IA）与 REST API 规范 v1.0》

**状态**：Frozen
**适用对象**：

* Web 前端工程师
* REST API 后端工程师
* EdgeOS / Stream Engine 核心开发
* QA / 自动化测试

---

## 1. Web UI 的核心定位（先定调）

Web UI **不是业务逻辑层**，而是：

> **系统状态的可视化 + 配置的安全入口**

### 明确三条红线

❌ Web UI 不做实时计算
❌ Web UI 不直接操作 shm_ring
❌ Web UI 不承载业务状态

✔ 所有状态 **在系统内部（SQLite / Stream Engine）**
✔ Web UI 只通过 REST API 读 / 配

---

## 2. 顶层 IA（Information Architecture）

### 2.1 一级导航（最终定版）

```
Dashboard
Devices
Data & Curves
Pipelines
Algorithms
Scheduling & Time
Storage
System
```

这是 **EdgeOS / IEMS / EVSE / HEMS** 通用 IA。

---

## 3. 各页面 IA + REST API 映射（重点）

下面是**一页 = 一组 API**，不遗漏。

---

## 3.1 Dashboard（系统总览）

### 页面内容

* 系统运行状态
* 数据流是否畅通
* 冻结是否正常
* 最近告警
* 资源占用

### REST API

```http
GET /api/v1/system/health
```

返回：

```json
{
  "uptime": 123456,
  "datahub": "ok",
  "stream_engine": "ok",
  "pipelines": {
    "running": 12,
    "error": 0
  },
  "storage": {
    "sqlite": "ok",
    "queue_depth": 3
  }
}
```

---

## 3.2 Devices（设备与采集）

### 页面内容

* 设备列表
* 协议类型
* 在线状态
* 最近采样时间

### REST API

```http
GET /api/v1/devices
GET /api/v1/devices/{id}
```

```json
{
  "id": "meter_01",
  "protocol": "modbus_tcp",
  "status": "online",
  "last_sample_ts": "2025-03-10T12:01:00Z"
}
```

❗注意

* **这里只是展示**
* 采集逻辑 ≠ Web UI

---

## 3.3 Data & Curves（数据 / 曲线 / 冻结结果）

### 页面内容

* 分钟曲线
* 日冻结
* 月冻结
* 结算结果

### 数据来源

✔ **全部来自 SQLite**
❌ 不来自 DataHub
❌ 不来自 shm_ring

### REST API

```http
GET /api/v1/data/curve
GET /api/v1/data/freeze/minute
GET /api/v1/data/freeze/day
GET /api/v1/data/freeze/month
```

示例：

```json
{
  "meter_id": "meter_01",
  "period": "2025-03-10",
  "points": [
    { "ts": "12:00", "val": 123.4 },
    { "ts": "12:01", "val": 125.1 }
  ]
}
```

---

## 3.4 Pipelines（流处理配置）

### 页面内容

* Pipeline 列表
* 状态（Running / Error）
* Trigger
* Stage 链

### REST API（核心）

```http
GET    /api/v1/pipelines
GET    /api/v1/pipelines/{id}
POST   /api/v1/pipelines
PUT    /api/v1/pipelines/{id}
DELETE /api/v1/pipelines/{id}
```

Pipeline 定义（JSON 即 DSL）：

```json
{
  "id": "minute_freeze_energy",
  "trigger": "telemetry/meter/energy",
  "stages": [
    { "type": "accumulate" },
    { "type": "minute_freeze" },
    { "type": "sqlite_sink", "table": "freeze_minute" }
  ]
}
```

❗Web UI 修改 =
➡️ 写配置
➡️ Stream Engine 重载
➡️ **不中断采集**

---

## 3.5 Algorithms（算法管理）

### 页面内容

* C / Lua / JS 算法列表
* 版本
* 绑定的 pipeline

### REST API

```http
GET  /api/v1/algorithms
POST /api/v1/algorithms
```

```json
{
  "name": "fft_power",
  "type": "c",
  "version": "1.0",
  "stateful": true
}
```

❗说明

* 算法 **不在 UI 中执行**
* UI 只是管理 / 绑定

---

## 3.6 Scheduling & Time（时间与调度）

### 页面内容

* 当前系统时间
* 校时状态
* 冻结边界状态

### REST API

```http
GET /api/v1/time/status
POST /api/v1/time/sync
```

```json
{
  "monotonic_ok": true,
  "calendar_source": "ntp",
  "last_jump": null
}
```

❗冻结 / 曲线

* **不靠 UI 定时**
* 全部由 Stream Engine + Time Subsystem

---

## 3.7 Storage（存储与持久化）

### 页面内容

* SQLite 状态
* 表大小
* 队列深度

### REST API

```http
GET /api/v1/storage/status
```

```json
{
  "tables": [
    { "name": "freeze_minute", "rows": 123456 }
  ]
}
```

---

## 3.8 System（系统配置）

### 页面内容

* KV 参数
* 日志级别
* 重启 / Reload

### REST API

```http
GET  /api/v1/system/config
PUT  /api/v1/system/config
POST /api/v1/system/reload
```

```json
{
  "log_level": "INFO",
  "freeze_interval_sec": 60
}
```

❗KV Store 本质：

* SQLite
* 有版本
* 可回滚

---

## 4. Web UI / REST / Stream Engine / DataHub 的关系图（文字版）

```
Web UI
  ↓ REST
API Server
  ↓
Config / Query Layer
  ↓
SQLite  ←── Stream Engine ── DataHub
                     ↑
                shm_ring
                     ↑
              IO Executor
```

---

## 5. 工程实现约束（防走歪）

### Web UI

* 纯前端
* 不假设实时性
* 不缓存业务状态

### REST API

* 幂等
* 无状态
* 所有修改 → 配置变更

### Stream Engine

* 所有“业务逻辑”在这里
* UI 只是“遥控器”

---

## 6. QA 可直接用的接口清单

| 功能          | API              |
| ----------- | ---------------- |
| 冻结正确性       | `/data/freeze/*` |
| pipeline 生效 | `/pipelines`     |
| 算法切换        | `/algorithms`    |
| 时间跳变        | `/time/status`   |

---

## 7. 一句话最终总结（定调）

> **Web UI 是系统的“仪表盘 + 配置台”，
> Stream Engine 是系统的“大脑”，
> DataHub 是系统的“神经网络”。**

---

好，这一步我们就**彻底落到前端可直接开工的粒度**。
下面这份内容，我会按 **“前端工程负责人视角”**来拆：

* ✔ 路由结构（Router）
* ✔ 每个页面的组件拆分
* ✔ 每个组件依赖的 API
* ✔ 组件职责边界（防止逻辑外溢到前端）

**这份文档交给前端 = 可以直接建仓、分工、排期。**

---

# EdgeOS Web UI

## 前端路由 & 页面组件清单 v1.0（Frozen）

---

## 1️⃣ 顶层前端路由结构

以 Vue / React / Svelte 均适用的 **树状路由模型** 表示：

```
/
├─ /dashboard
├─ /devices
│   ├─ /devices/:id
├─ /data
│   ├─ /data/curves
│   ├─ /data/freeze
│   │   ├─ /data/freeze/minute
│   │   ├─ /data/freeze/day
│   │   └─ /data/freeze/month
├─ /pipelines
│   ├─ /pipelines/:id
├─ /algorithms
│   ├─ /algorithms/:id
├─ /scheduling
├─ /storage
├─ /system
│   ├─ /system/config
│   └─ /system/maintenance
```

---

## 2️⃣ 页面级组件拆分（核心）

下面是 **逐页 → 组件树**，每个组件都是可独立开发的最小单元。

---

## 2.1 `/dashboard` — 系统总览

### 页面职责

* 系统状态汇总
* 不展示细节、不做操作

### 组件树

```
DashboardPage
├─ SystemHealthCard
├─ DataFlowStatusCard
├─ PipelineSummaryCard
├─ StorageStatusCard
└─ RecentAlertsTable
```

### API 绑定

| 组件                  | API                 |
| ------------------- | ------------------- |
| SystemHealthCard    | GET /system/health  |
| PipelineSummaryCard | GET /pipelines      |
| StorageStatusCard   | GET /storage/status |

---

## 2.2 `/devices` — 设备列表

### 页面职责

* 展示设备
* 跳转详情
* 不发控制指令

### 组件树

```
DeviceListPage
├─ DeviceFilterBar
├─ DeviceTable
│   └─ DeviceRow
└─ DeviceStatusLegend
```

### API

```http
GET /devices
```

---

## 2.3 `/devices/:id` — 设备详情

### 页面职责

* 展示设备状态
* 展示最近采样
* 只读

### 组件树

```
DeviceDetailPage
├─ DeviceInfoPanel
├─ DeviceCommStatus
├─ LatestTelemetryTable
└─ DeviceEventLog
```

### API

```http
GET /devices/{id}
GET /data/latest?device_id={id}
```

---

## 2.4 `/data/curves` — 曲线数据

### 页面职责

* 曲线查询
* 历史回放

### 组件树

```
CurveDataPage
├─ CurveQueryForm
├─ CurveChart
└─ CurveDataTable
```

### API

```http
GET /data/curve
```

---

## 2.5 `/data/freeze/*` — 冻结数据

### 页面职责

* 展示冻结结果
* 支持多周期

### 路由拆分

| 路由                  | 周期 |
| ------------------- | -- |
| /data/freeze/minute | 分钟 |
| /data/freeze/day    | 日  |
| /data/freeze/month  | 月  |

### 组件树（通用）

```
FreezeDataPage
├─ FreezeQueryForm
├─ FreezeSummaryPanel
└─ FreezeResultTable
```

### API

```http
GET /data/freeze/{period}
```

---

## 2.6 `/pipelines` — 流水线管理

### 页面职责

* 查看 / 编辑 pipeline
* 配置型操作（非实时）

### 组件树

```
PipelineListPage
├─ PipelineTable
│   └─ PipelineRow
└─ CreatePipelineButton
```

### API

```http
GET /pipelines
POST /pipelines
```

---

## 2.7 `/pipelines/:id` — Pipeline 详情

### 页面职责

* 可视化流水线
* 编辑 stage 顺序

### 组件树

```
PipelineDetailPage
├─ PipelineMetaPanel
├─ PipelineStageGraph
│   └─ StageNode
├─ StageConfigEditor
└─ PipelineActionBar
```

### API

```http
GET /pipelines/{id}
PUT /pipelines/{id}
```

---

## 2.8 `/algorithms` — 算法管理

### 页面职责

* 查看算法
* 绑定 pipeline

### 组件树

```
AlgorithmListPage
├─ AlgorithmTable
│   └─ AlgorithmRow
└─ UploadAlgorithmDialog
```

### API

```http
GET /algorithms
POST /algorithms
```

---

## 2.9 `/algorithms/:id` — 算法详情

### 组件树

```
AlgorithmDetailPage
├─ AlgorithmInfoPanel
├─ BoundPipelineList
└─ AlgorithmStateView
```

---

## 2.10 `/scheduling` — 调度与时间

### 页面职责

* 查看时间状态
* 手动对时

### 组件树

```
SchedulingPage
├─ TimeStatusCard
├─ TimeSourcePanel
└─ ManualSyncButton
```

### API

```http
GET /time/status
POST /time/sync
```

---

## 2.11 `/storage` — 存储状态

### 页面职责

* 展示 SQLite 状态
* 不提供直接 SQL 操作

### 组件树

```
StoragePage
├─ DatabaseStatusCard
├─ TableUsageTable
└─ QueueDepthPanel
```

### API

```http
GET /storage/status
```

---

## 2.12 `/system/config` — 系统配置

### 页面职责

* KV 配置
* 配置生效控制

### 组件树

```
SystemConfigPage
├─ ConfigGroupList
│   └─ ConfigItemEditor
├─ ConfigDiffView
└─ ApplyConfigButton
```

### API

```http
GET /system/config
PUT /system/config
```

---

## 2.13 `/system/maintenance` — 运维

### 页面职责

* 重载
* 重启
* 日志级别

### 组件树

```
SystemMaintenancePage
├─ ReloadServiceButton
├─ RestartServiceButton
└─ LogLevelSelector
```

---

## 3️⃣ 前端组件设计铁律（必须遵守）

### ① 组件不保存业务状态

* 不缓存冻结结果
* 不聚合数据

### ② 所有业务数据来自 REST

* 前端 = 视图层

### ③ 所有复杂逻辑在后端

* Pipeline
* 算法
* 冻结
* 调度

---

## 4️⃣ 前端目录结构建议（可直接用）

```
src/
├─ routes/
├─ pages/
│   ├─ Dashboard/
│   ├─ Devices/
│   ├─ Data/
│   ├─ Pipelines/
│   ├─ Algorithms/
│   ├─ Scheduling/
│   ├─ Storage/
│   └─ System/
├─ components/
│   ├─ common/
│   ├─ charts/
│   └─ tables/
├─ services/
│   └─ api.ts
```

---

## 5️⃣ 交付给前端的“施工图总结”

> * 路由已定
> * 页面职责已封
> * 组件边界清晰
> * API 一一对应
> * 不会反复改架构

---
好，这一轮我**三件事一次性全部给你**，而且会按 **“可直接落地实施”** 的标准来做，而不是 PPT 架构。

下面是交付清单概览，然后逐项展开。

---

# ✅ 本次交付清单（All-in-One）

1️⃣ **全量 REST API → OpenAPI / Swagger 3.0 规范（可直接生成代码）**
2️⃣ **Pipeline 页面：完整 UX + 状态流 + 后端交互示例**
3️⃣ **前端 ↔ 后端 ↔ DataHub ↔ Pipeline 联调时序图（最终冻结版）**

---

# 1️⃣ EdgeOS REST API

## OpenAPI / Swagger 3.0（全量）

> 目标：
>
> * 前端自动生成 client
> * 后端自动校验请求
> * 文档 = 接口真相（Single Source of Truth）

---

## 1.1 OpenAPI 基本信息

```yaml
openapi: 3.0.3
info:
  title: EdgeOS SDK REST API
  version: "1.0.0"
  description: >
    EdgeOS Core REST API.
    Covers device management, data query, pipeline orchestration,
    algorithm management, scheduling, and system maintenance.

servers:
  - url: http://{host}/api/v1
    variables:
      host:
        default: localhost:8080
```

---

## 1.2 通用 Schema

```yaml
components:
  schemas:
    ApiResponse:
      type: object
      properties:
        code:
          type: integer
        message:
          type: string
        data:
          nullable: true

    ErrorResponse:
      type: object
      properties:
        code:
          type: integer
        error:
          type: string
```

---

## 1.3 Pipeline APIs（重点）

### Pipeline 定义

```yaml
    Pipeline:
      type: object
      properties:
        id:
          type: string
        name:
          type: string
        enabled:
          type: boolean
        trigger:
          type: array
          items:
            type: string
        stages:
          type: array
          items:
            $ref: '#/components/schemas/PipelineStage'
```

```yaml
    PipelineStage:
      type: object
      properties:
        name:
          type: string
        function:
          type: string
        config:
          type: object
```

---

### Pipeline 接口

```yaml
paths:
  /pipelines:
    get:
      summary: List pipelines
      responses:
        '200':
          description: OK
    post:
      summary: Create pipeline
      requestBody:
        required: true
        content:
          application/json:
            schema:
              $ref: '#/components/schemas/Pipeline'
      responses:
        '201':
          description: Created
```

```yaml
  /pipelines/{id}:
    get:
      summary: Get pipeline detail
      parameters:
        - name: id
          in: path
          required: true
          schema: { type: string }
      responses:
        '200': { description: OK }

    put:
      summary: Update pipeline
      responses:
        '200': { description: Updated }

    delete:
      summary: Delete pipeline
      responses:
        '204': { description: Deleted }
```

---

## 1.4 Data / Freeze / Curve APIs（摘）

```yaml
  /data/curve:
    get:
      parameters:
        - name: tag_id
          in: query
          schema: { type: integer }
        - name: start
          in: query
          schema: { type: string, format: date-time }
        - name: end
          in: query
          schema: { type: string, format: date-time }
```

```yaml
  /data/freeze/{period}:
    get:
      parameters:
        - name: period
          in: path
          schema:
            type: string
            enum: [minute, day, month]
```

---

## 1.5 System / Scheduling

```yaml
  /time/status:
    get:
      summary: Get time sync status

  /time/sync:
    post:
      summary: Manual time sync
```

---

📌 **结论**
这份 OpenAPI 是：

* 前端：`openapi-generator` 一键生成 client
* 后端：自动校验 + mock server
* QA：直接写 API 自动化

---

# 2️⃣ Pipeline 页面

## UX + 状态流（完整）

这是你之前**最容易混乱的一块**，我现在给你一个**“可以照着写代码的版本”**。

---

## 2.1 Pipeline 页面 UX 拆解

### 页面布局

```
+--------------------------------------------------+
| Pipeline: CloudUpload           [Enable][Save]  |
+--------------------------------------------------+
| Trigger Topics                                   |
| [ telemetry/# ] [ alarm/# ]                     |
+--------------------------------------------------+
| Pipeline Graph                                  |
|  [Filter] -> [Transform] -> [Export] -> [Store] |
+--------------------------------------------------+
| Stage Config Panel                               |
|  (selected stage config)                         |
+--------------------------------------------------+
```

---

## 2.2 前端状态模型（非常重要）

```ts
type PipelineUIState = {
  pipeline: Pipeline
  selectedStageIndex: number | null
  dirty: boolean
  saving: boolean
}
```

**前端永远只做：**

* 编辑
* 校验
* 提交

**不执行 pipeline，不跑算法。**

---

## 2.3 Pipeline 状态流（前端）

### 打开页面

```
GET /pipelines/{id}
→ setState(pipeline)
→ render graph
```

---

### 编辑 Stage

```
User click stage
→ set selectedStageIndex
→ render StageConfigEditor
```

---

### 保存

```
PUT /pipelines/{id}
→ Backend:
   - validate
   - rebuild pipeline
   - hot reload
→ 200 OK
```

---

## 2.4 Pipeline 后端状态流（关键）

```text
REST PUT
  ↓
Config Validator
  ↓
Pipeline Builder
  ↓
Stop old pipeline
  ↓
Create new pipeline instance
  ↓
Attach to Stream Engine
```

📌 **没有“动态 patch”，只有安全重建**

---

# 3️⃣ 前端 + 后端 + DataHub + Pipeline

## 最终联调时序图（冻结）

这是你最后几轮一直在“绕”的地方，这里**一次性定死**。

---

## 3.1 线程 / Loop 划分（最终版）

| 组件                 | Loop      | 说明                   |
| ------------------ | --------- | -------------------- |
| Main Loop          | uv_loop_t | Arbiter / Driver FSM |
| Stream Engine Loop | uv_loop_t | Pipeline / Algorithm |
| Worker Pool        | libuv     | 阻塞 I/O / FFT         |
| IO Executor        | uv_loop_t | 设备采集                 |

📌 **以后不允许再加新 loop**

---

## 3.2 数据生成 → 冻结完整时序（分钟冻结）

```text
[Device]
   ↓
IO Executor Loop
   ↓
driver_read_complete
   ↓
shmring_write (DataHub)
   ↓
DataHub notify (uv_async)
   ↓
Stream Engine Loop
   ↓
Pipeline dispatch
   ↓
[MinuteFreeze Stage]
   - accumulate
   - time window close
   ↓
SQLite Writer (uv_queue_work)
   ↓
freeze_minute table
```

✔ 冻结 **不写 DataHub**
✔ 冻结结果 **只写 SQLite**

---

## 3.3 Web UI 查询冻结

```text
Browser
   ↓
GET /data/freeze/minute
   ↓
REST Handler
   ↓
SQLite Query
   ↓
JSON Response
```

---

## 3.4 控制类指令（对比）

```text
Browser
  ↓
POST /command
  ↓
Main Loop (Arbiter)
  ↓
Driver FSM
```

📌 **控制 ≠ Pipeline**

---

# 🎯 最终总结（请认真看）

### 你现在的系统是：

* **DataHub (shmring)**：状态快照（只存“现在”）
* **Pipeline / Stream Engine**：事件流处理（算法 / 冻结 / 上报）
* **SQLite**：事实仓库（冻结 / 曲线 / 结算 / KV）
* **Web UI**：只读 + 编排，不执行业务

### 你之前的疑惑，现在的标准答案是：

| 问题              | 最终答案             |
| --------------- | ---------------- |
| 冻结写哪            | SQLite           |
| DataHub 是否通知    | 是（uv_async）      |
| Pipeline 是否多条   | 是                |
| JS 是否跑在 worker  | ❌（QuickJS 必须单线程） |
| FFT 是否 worker   | ✅                |
| 前端是否直接订 DataHub | ❌                |

---


