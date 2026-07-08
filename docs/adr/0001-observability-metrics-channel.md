# ADR-0001: DataHub 内建可观测性 Metrics 通道

**状态**：已批准 (Approved)  
**日期**：2026-07-06  
**决策者**：EdgeC 架构团队  
**Grilling 决议**：2026-07-06 通过 4 项设计决策（见"设计决策"章节）

## 背景

EdgeC 运行时当前是一个"黑盒"：

- C 引擎内部没有任何结构化 metrics 暴露（队列深度、处理延迟、丢包率、Stage 处理时间）
- 调试依赖本地 `zlog` 日志输出去 Console / Syslog
- Go 端访问 DataHub 依赖 100Hz 轮询共享内存，在标签数扩大至数万时 CPU 开销线性增长
- 没有标准化的健康检查或性能诊断协议
- 没有为 SDK 分级许可（Basic/Professional/Enterprise）提供用量计量数据

## 选项

### 选项 A：DataHub 内建系统 Metrics 通道

在 `PipeMsg` 中增加一个 `PIPEMSG_TYPE_METRIC` 类型。引擎内部各模块周期性写入结构化的性能指标，以标准的 EdgeC 消息格式存在于 Ring Buffer 中。Go 端通过已有 SHM 读取机制消费，暴露为 `/metrics` HTTP 端点（Prometheus 格式）。

**优点：**
- 复用已有的零拷贝 DataHub 基础设施，无需引入新 IPC
- 引擎内部各模块天然可见（Arbiter 队列、SnF 缓冲深度、Stage 延迟）
- Go / UI / Edge-Ops 统一通过同一个 HTTP API 消费
- 为 License 引擎提供运行时用量数据通道

**缺点：**
- Metric 数据与业务数据共用 Ring Buffer，高负载时可能互相争用槽位
- Metric 写入频率需要精心设计，否则高频率采样会引发 Ring Buffer 翻覆

### 选项 B：Sidecar 进程 + Prometheus C SDK

在 EdgeC 引擎外部启动一个独立的监测进程，通过 POSIX 信号或 `eventfd` 钩子采集数据，通过标准的 Prometheus C Client 库暴露 `/metrics`。

**优点：**
- 与业务数据完全隔离，不影响数据平面性能
- 可以使用标准 Prometheus 生态

**缺点：**
- 引入新的外部依赖（Prometheus C Client）
- Sidecar 进程增加了部署复杂度，不适合 MCU 部署场景
- 无法观测引擎内部精细粒度的每个 Stage 处理时间（跨进程边界采集丢失精度）
- 与 Basic 三级（Cortex-M4）不兼容

### 选项 C：zlog 结构化日志 + 日志聚合

在现有 `zlog` 基础上增加 JSON 结构化日志输出，由外部日志聚合工具（Loki / ELK）解析为 metrics。

**优点：**
- 改动最小，只需扩展现有 `edge_log.h`
- 运维人员熟悉 ELK 方案

**缺点：**
- 日志采集非实时，无法用于 License 计量
- 高吞吐时日志 IO 成为瓶颈
- 结构化日志解析的 CPU 开销在 MCU 上不可接受
- 无法暴露细粒度数值指标（队列深度、延迟百分位数）

## 决策

选择 **选项 A：DataHub 内建系统 Metrics 通道**。

**理由：**

1. **零拷贝一致性**：DataHub 已经是所有 IPC 的统一介质，不应为 metrics 引入第二条路径
2. **MCU 兼容**：选项 A 不需要额外进程或外部库，适用于 Basic 三级（Cortex-M4）
3. **精细度**：引擎内部模块可以直接写 `PIPEMSG_TYPE_METRIC`，无需跨进程边界
4. **统一数据源**：License 引擎可以从此通道读取用量数据，无需单独埋点

## 设计决策（Grilling 2026-07-06）

| # | 问题 | 决策 |
|---|------|------|
| G1 | Ring Buffer 满时策略 | 丢 Metric，不阻塞业务数据 |
| G2 | 采样频率 | 两级制：高频环 100ms（队列/延迟），低频环 10s（诊断快照） |
| G3 | 指标 Tag ID 空间 | 12000-12999，按模块分区注册（DataHub/StreamEngine/Arbiter/SnF/License） |
| G4 | Shadow Table 存储 | Metric 写 Shadow Table，Go 端 `ReadTag()` 零改动 |

## 实施要点

1. 在 `contract_data_types.h` 中新增 `PIPEMSG_TYPE_METRIC` 枚举值
2. 定义 Metrics 标签 ID 范围（12000-12999），区别于业务标签
3. 各核心模块按需注册 Metric 标签（Arbiter / DataHub / SnF / StreamEngine）
4. Go API Core 端新增 `/api/v1/system/metrics` 端点，格式化为 Prometheus 文本格式
5. Metrics 写入采用"尽力而为"模式——Ring Buffer 满时丢弃 metric，不影响业务数据

## 影响

- **正面**：C 引擎首次具备结构化可观测性，License 计量有数据来源
- **正面**：Edge-Ops 云端可以直接消费标准 metrics 接口
- **负面**：Metric 标签占用标签 ID 空间
- **负面**：每个模块需要额外 ~100 行埋点代码

## 替代方案回顾

选项 C（日志聚合）作为补充方案保留——结构化的 `edge_log.h` 格式化输出可以配合 Loki 做事件日志的检索。但与 Metrics 通道是互补关系，不冲突。

## 参考文献

- EdgeC `contract_data_types.h` — PipeMsg 类型枚举
- `MONETIZATION_STRATEGY.md` — License 引擎依赖用量数据
