# EdgeC 实时系统运行动态方案与计划（全产品必选）

> 日期：2026-11-21　状态：草案，待评审后派活
> 读者：前端、C 开发、验证、产品
> 用法：§6 的任务表是派活单位，一任务一 issue 一 PR；§7 定跟踪口径。本能力对全部产品与全部功能是必选项。

## How to read this

一个任务行就是一个工作单元。每行都写了负责角色、估时、依赖、交付物和出口证据。一个任务就是一个工作单元，每个工作单元都写明检查它的证据。出口证据必须是能复现的东西：测试输出、抓包、截图、PR 链接或提交 SHA。没有证据就不算完成。

测试通过只是必要条件。完成定义是仓库口径：静态分析、测试、运行时验证证据三者齐全（`AGENTS.md`）。落到每个 PR 上，就是单元、实机与性能三类证据都齐了才算验过。Tests alone are not sufficient verification. A PR is verified only when its unit, live, and perf boxes are all checked.

## Program checklist

- [ ] 评审本方案，§9 的开放问题 1、2、4、6 给出结论。
- [ ] 按 §6 给 P0 到 P7 建 GitHub Milestone。
- [ ] 按 §7.1 给每个任务建 issue，挂里程碑与标签。
- [ ] 每周一认领，周五交证据并更新进度表。
- [ ] 每个 PR 合入后，在目标分支重跑 `run_guard_selftest.py` 与前端测试。

## 1. 结论

WebUI 现在能看快照，看不到动态。前端实时通道的代码已经写好，服务端却没有这个接口；页面全部靠 REST 快照刷新。要做「DCU 实时抄表动态」，需要补齐三件事：

1. 服务端的实时推送通道（首版用 SSE），带背压、合并与序号。
2. 产品侧的事件桥：把 DataHub 与抄表调度的事件推给通道，并提供一份快照接口用于重同步。
3. 前端传输层与一个动态图页面：按通道、表计显示空闲、抄读中、成功、失败，附吞吐与时延。
4. 指标暴露：ADR-0001 的指标通道已有注册 API 却没有消费端，补 Prometheus 文本端点与看板，动态图上的吞吐与时延才有稳定数据源。

复用现有前端契约（`LiveMessage`、topic、seq 与 resync 语义），不另起一套。

### 1.1 必选范围与覆盖矩阵

实时运行动态是产品能力，不是某个页面的装饰。每个产品都要能回答三个问题：现在在做什么，做到哪一步，卡在哪里。因此以下四项对每个产品、每个功能模块都是必选：

1. 事件源要注册 topic，覆盖该模块的主要状态变化。
2. 快照接口要能返回当前状态，供首次加载与断流重同步。
3. 指标要注册计数与直方图，供 Grafana 与告警使用。
4. 前端要有对应面板，由产品清单驱动，不允许某个产品靠「还没做」跳过。

| 产品 | 必选面板 | 事件源 | 指标 |
| --- | --- | --- | --- |
| DCU | 抄表网络、通道状态、事件与告警、升级进度 | 轮询阶段、通道变化、事件 | 成功率、超时、活跃表数、时延 |
| IEMS | 采集链路、控制下发、储能簇状态、应用运行 | 采集、控制、应用状态机 | 采集周期、控制时延、应用重启 |
| SmartBox | 通道与断路单元、环境感知、双授权操作 | 通道、脱扣、授权流程 | 通道可用率、脱扣次数、授权时延 |
| 配网终端（规划） | 四遥通道、SOE、FA 过程 | 遥测遥信、命令、FA 状态 | 命令往返、SOE 溢出、FA 时长 |
| 平台能力 | app-host 应用、北向投递、SnF 队列、存储 | 应用状态、投递与积压 | 投递成功率、积压深度、写放大 |

强制方式落在可执行检查上，不靠文档自觉：产品清单 `product.yaml` 增加 `livePanels` 必填项，`web/tooling/lib/schema.mjs` 校验；仓库新增 `check_live_coverage.py` 校验每个产品都有事件源注册、快照与指标，正反夹具登记进 `run_guard_selftest.py`。

## 2. 现状盘点

| 能力 | 现状 | 证据 |
| --- | --- | --- |
| 前端实时客户端 | 已有 WebSocket 客户端，默认 `/api/v1/ws`，断线指数退避 | `web/core/src/sdk/live.ts` |
| 前端实时 store | 已有，topic 为 alarm/task/config-transaction/telemetry，seq 断档触发 resync | `web/core/src/app/stores/live.ts` |
| 启动时机 | `AppShell` 挂载即 `live.start()`，注释说「已登录才建立」，代码没有判断 | `web/core/src/app/layouts/AppShell.vue:44-48` |
| 服务端推送 | 没有。`rest_server.c` 无 WebSocket、无 `text/event-stream` | 全文件检索 |
| 快照接口 | 有单点最新值 `/api/v1/data/latest/<tag>` | `adapters/rest_api/src/rest_server.c:921` |
| DCU 抄表 | 轮询器与最新值表已就绪 | `cmd/dcu/dcu_poller.c`、`cmd/dcu/dcu_storage.c:51`（`dcu_telemetry_latest`） |
| DCU 页面 | 只有电表档案页，取 `/api/v1/dcu/meters` | `web/products/dcu/pages/MetersView.vue:31` |
| 该路由的服务端实现 | 不存在，只有 mock 有 | `web/mock-server/server.mjs:1461`；`adapters/rest_api` 无此路由 |
| 可视化积木 | 以热力图、拓扑、报文抓包命名的组件已有 | `web/core/src/modules/devices/ChannelTopologyHeatmap.vue`、`web/core/src/modules/dcu/PacketCapture.vue` |
| 设计意图 | 明确弃短轮询，走 WebSocket 或 SSE | `docs/design/edgeos_web_ui_ux_masterplan.md:87` |

两个已确认的缺口，计划内一并处理：AppShell 无条件连一个不存在的地址，会在控制台持续重连；电表档案页依赖的路由服务端没有实现。

## 3. 实时契约

复用前端已有的 `LiveMessage`，不改字段。

```
{ "type": "event", "seq": 1024, "ts": "2026-11-21T10:00:00.123Z",
  "topic": "task", "payload": { ... } }
```

规则：

- `seq` 单调递增，服务端不保留积压。客户端发现断档就整体重取快照，不能假设连续。
- topic 首版四个：`telemetry`、`task`、`alarm`、`config-transaction`。动态图只用前两个。
- 载荷（示例）：

| topic | 事件 | 载荷 |
| --- | --- | --- |
| `task` | 抄表开始 | `{bus_id, meter, channel, started_at}` |
| `task` | 抄表完成 | `{bus_id, meter, latency_ms, ok}` |
| `task` | 抄表失败 | `{bus_id, meter, latency_ms, error}` |
| `telemetry` | 点值变化 | `{point, value, quality, ts}`，按点合并 |
| `alarm` | 告警 | 沿用现有告警对象 |

背压与合并：服务端每 100 ms 合并一次，同一表计同一阶段只保留最后一条；每客户端队列上限 256 条，满则丢最旧并计入 `dropped`；单条事件上限 8 KB；每 15 秒发一次注释行保活。

## 4. 架构与组件

```
DataHub / dcu_poller ──publish──> live_feed 适配器（SSE）──> 浏览器 EventSource
        ▲                                   ▲
        │ 快照 GET /api/v1/dcu/polling/state │ 配置与限流
   产品装配（cmd/dcu）                       组合根
```

新组件 `adapters/live_feed`，端口放在 `foundation/include/live_feed_ports.h`，消费者定义：

```
int live_feed_publish(live_feed_t *feed, const char *topic, const char *json, size_t len);
```

产品只在装配时创建它并把 `publish` 注入事件源；适配器负责监听、订阅过滤、合并、限流、SSE 编码与断线清理。这样其他产品（IEMS、SmartBox）可以复用同一个适配器，只换事件源。

前端改动集中在两处：`web/core/src/sdk/sse.ts` 新增 SSE 传输（与 `live.ts` 同一 `LiveChannel` 接口），`stores/live.ts` 按配置选择传输（`sse` 默认，`ws` 兼容，`poll` 降级为 2 秒快照）。`AppShell` 的启动加登录与能力判断。

页面分两层。通用层是 `web/core/src/modules/live/LiveDynamics.vue`，按 `product.yaml` 的 `livePanels` 渲染卡片、状态格、吞吐与时延，任何产品接上事件源就可用。产品专用层只在通用层表达不了时新增，例如 DCU 的抄表网络与 IEMS 的储能簇。

页面放在 `web/core/src/modules/live/MeterNetworkLive.vue`，DCU 与 IEMS 都能挂。展示内容：按通道分组的表计网格或拓扑，四种状态（空闲、抄读中、成功、失败），吞吐折线，时延分位数，失败明细。状态来自快照，动画来自事件。

## 5. 关键设计决定

### 5.1 首版用 SSE，不用 WebSocket

| 维度 | SSE | WebSocket |
| --- | --- | --- |
| 服务端实现量 | 纯 HTTP chunked，约 300 行，含限流 | 需 RFC 6455 握手、掩码、分片、ping/pong，约 800 行且更易错 |
| 方向 | 服务端到客户端，正好匹配只读推送 | 双向，本项目控制走 REST，用不上 |
| 断线重连 | 浏览器自带 | 需自己实现（现有客户端已实现） |
| 代理与防火墙 | 普通 HTTP，兼容好 | 需要 Upgrade 头，部分代理会拦 |
| 现有前端 | 需加一个传输实现 | 已实现 |

结论：首版 SSE。前端保留 `LiveChannel` 接口，后续要做双向再补 WS，不改调用方。

### 5.2 事件量估算

1000 只表、每只每分钟抄一次，平均约 17 条事件每秒，尖峰出现在轮询批次启动时。按 100 ms 合并、每客户端 256 条队列，8 个客户端时内存上界约 2 MB。这个量级 SSE 足够，不需要引入消息中间件。

### 5.3 安全

现有 REST 没有任何鉴权（`rest_server.c` 无 Cookie、Bearer 或权限检查），实时通道沿用它：绑定本机地址或放在网关后。若后续加鉴权，在订阅时校验，不在事件层。

### 5.4 与指标、链路追踪的关系

实时推送和指标不是二选一。前者是事件级的，给页面画动画；后者是聚合与趋势，给值班、告警和对外监控。两者数据源相同，用途不同。

| 需求 | 推送（本方案） | 指标 | 链路追踪 |
| --- | --- | --- | --- |
| 页面上的表计状态动画 | 需要，事件级 | 不需要 | 不需要 |
| 成功率、时延分位、活跃表数 | 可用，但页面不在时无人记录 | 需要，可被 Prometheus 抓取并告警 | 不需要 |
| 对外 Grafana 大屏与历史 | 不保留历史 | 需要 | 不需要 |
| 定位一次慢抄表落在哪一段 | 只能看结果 | 只能看分布 | 需要时才有用 |

仓库现状：ADR-0001 已批准，指标通道 API 已实现（`modules/edge_core/include/edge_metrics.h`，DataHub 已注册 12001 到 12004，见 `modules/edge_core/src/datahub_shm.c:180`），但**没有消费端**：没有任何地方读 `PIPEMSG_TYPE_METRIC`，也没有 `/metrics` 或 `/api/v1/system/metrics` 的处理器。`MetricsMatrix.vue` 只写了端点标签，没有取数逻辑。链路追踪全仓没有实现。

结论：

- 指标要做，而且是本方案的前置：先有计数与直方图，动态图上的吞吐与时延才有稳定数据源，值班也有告警依据。首版给 Prometheus 文本端点，不引入客户端库；Grafana 只交付看板 JSON 与部署说明，不做嵌入式图形栈。
- 链路追踪首版不做。单进程 C 运行时加 app-host，一次抄读的路径短，用计数器加事件时延就能定位；OTLP 导出器、span 传播与采样策略是另一个量级的依赖，等出现跨进程或跨产品联调的真实需求再立项。
- 指标与推送共用一套计数来源：产品侧把计数器注册到 `edge_metrics`，推送适配器与指标端点各取所需，避免两套口径。

### 5.5 业界工具的位置

用户问到的类似项目按用途分开：

| 工具 | 擅长 | 在本项目的用法 |
| --- | --- | --- |
| Grafana | 看板与告警，接 Prometheus 等数据源 | 已有指标端点的消费端，交付看板 JSON |
| Netdata | 单机秒级采集，自带面板与告警，开箱即用 | 可选的主机级观测，不进入产品交付，也不替代产品级事件 |
| Kibana | 日志检索与可视化，依赖 Elasticsearch | 本项目日志走 zlog，不引入 ELK |
| SigNoz | 基于 OpenTelemetry 的链路与服务指标 | 与追踪一起评估，首版不引入 |
| Zabbix | 传统设备与网络监控，SNMP 与模板 | 运维侧可选，不嵌入产品 |

结论不变：产品内的实时动态由本方案的推送与指标承担；Grafana 是外部看板；Netdata 与 Zabbix 属于现场运维工具，不作为交付依赖。

## 6. 工作分解与排期

角色：A 架构，C C 开发，D 驱动或数据侧，F 前端，Q 验证，P 产品。人日为估算。

### P0 契约与现状冻结（4 人日，第 1 周）

| ID | 任务 | 角色 | 人日 | 依赖 | 交付物 | 出口证据 |
| --- | --- | --- | --- | --- | --- | --- |
| LIV-P0-01 | 实时契约冻结（事件、seq、合并、限流） | A+F | 2 | 无 | 契约文档与类型定义 | 评审记录，前端类型可编译 |
| LIV-P0-02 | 事件源盘点（DCU 轮询与 DataHub 可订阅点） | D | 1 | 无 | 事件清单与采样频率 | 清单评审 |
| LIV-P0-03 | 页面需求与非功能指标（帧率、事件率、时延） | P+Q | 1 | 01 | 指标表 | 指标表评审 |

### P1 服务端 SSE 适配器（12 人日，第 2 到 3 周）

| ID | 任务 | 角色 | 人日 | 依赖 | 交付物 | 出口证据 |
| --- | --- | --- | --- | --- | --- | --- |
| LIV-P1-01 | `live_feed_ports.h` 端口头 | A | 2 | P0-01 | 端口与类型 | 端口评审，无协议类型泄漏 |
| LIV-P1-02 | `adapters/live_feed` 组件（监听、订阅过滤、合并、限流、SSE 编码、保活） | C | 6 | 01 | 组件与 `component.meta` | 契约测试：订阅、断线、队列满、保活 |
| LIV-P1-03 | 客户端上限与丢弃计数可观测 | C | 2 | 02 | 计数接口与日志 | 压测下计数正确 |
| LIV-P1-04 | 组件门禁登记（边界、预算、目标、自检夹具） | C | 2 | 02 | 门禁通过记录 | `run_guard_selftest.py` 绿 |

### P2 DCU 事件源与快照（10 人日，第 3 到 4 周）

| ID | 任务 | 角色 | 人日 | 依赖 | 交付物 | 出口证据 |
| --- | --- | --- | --- | --- | --- | --- |
| LIV-P2-01 | 轮询事件钩子（开始、完成、失败、时延） | D | 4 | P1-01 | `dcu_poller` 事件发布 | 单测：每阶段一条事件 |
| LIV-P2-02 | 遥测合并事件（按点 200 ms 合并） | D | 2 | 01 | 事件桥 | 合并率与丢包计数用例 |
| LIV-P2-03 | 快照接口 `/api/v1/dcu/polling/state` | C | 2 | 无 | REST 处理器 | 契约测试，含空库 |
| LIV-P2-04 | 补 `/api/v1/dcu/meters` 服务端实现 | C | 2 | 无 | REST 处理器 | 与 mock 返回结构一致 |

### P3 前端通道（10 人日，第 4 到 5 周）

| ID | 任务 | 角色 | 人日 | 依赖 | 交付物 | 出口证据 |
| --- | --- | --- | --- | --- | --- | --- |
| LIV-P3-01 | SSE 传输（`sdk/sse.ts`，同一 `LiveChannel` 接口） | F | 4 | P1-02 | 传输实现 | 单测：重连、resync、脏数据丢弃 |
| LIV-P3-02 | store 传输选择与降级（sse 默认、ws 兼容、poll 兜底） | F | 2 | 01 | 配置与选择逻辑 | 三种模式切换用例 |
| LIV-P3-03 | AppShell 启动条件（登录与能力判断） | F | 1 | 01 | 启动门控 | 未登录不发起连接 |
| LIV-P3-04 | mock server 支持 SSE 与事件脚本 | F | 3 | 01 | mock 端点 | 本地开发可复现动态 |

### P4 动态图页面（12 人日，第 5 到 6 周）

| ID | 任务 | 角色 | 人日 | 依赖 | 交付物 | 出口证据 |
| --- | --- | --- | --- | --- | --- | --- |
| LIV-P4-01 | 页面骨架：按通道分组的表计网格与状态色 | F | 4 | P2-03、P3-02 | `MeterNetworkLive.vue` | 截图，四种状态可见 |
| LIV-P4-02 | 事件驱动动画与合并（100 ms 帧合并） | F | 3 | 01 | 动画层 | 截图对比，帧率达标 |
| LIV-P4-03 | 吞吐与时延面板 | F | 2 | 01 | 折线与小指标 | 截图，数据与快照一致 |
| LIV-P4-04 | 失败明细与筛选 | F | 2 | 01 | 列表与筛选 | 截图，筛选生效 |
| LIV-P4-05 | 可访问性与降级（无事件时显示快照与提示） | F | 1 | 01 | 降级视图 | 断流截图 |

### P5 验证与推广（6 人日，第 6 周）

| ID | 任务 | 角色 | 人日 | 依赖 | 交付物 | 出口证据 |
| --- | --- | --- | --- | --- | --- | --- |
| LIV-P5-01 | Playwright E2E：真产品加 SSE，覆盖四态与断流重连 | Q | 3 | P4-02、P4-05 | E2E 用例 | 截图与视频归档 |
| LIV-P5-02 | 性能：1000 表仿真下的浏览器 CPU 与事件吞吐 | Q | 2 | 01 | 性能记录 | 指标达标或不达标的结论 |
| LIV-P5-03 | 复用说明：IEMS 接入事件源的步骤 | A | 1 | P2 | 接入文档 | 文档评审 |

### P6 指标暴露与看板（12 人日，第 7 周）

| ID | 任务 | 角色 | 人日 | 依赖 | 交付物 | 出口证据 |
| --- | --- | --- | --- | --- | --- | --- |
| LIV-P6-01 | DCU 计数器与直方图注册（成功、超时、活跃表、时延桶） | D | 4 | P2-01 | 计数器注册与上报 | 计数随仿真变化，指标名稳定 |
| LIV-P6-02 | Prometheus 文本端点 `/api/v1/system/metrics` | C | 4 | 01、P2-03 | REST 处理器 | 抓取返回合法文本格式 |
| LIV-P6-03 | Grafana 看板 JSON 与部署说明 | C+Q | 2 | 02 | 看板文件与 README | 导入后四张图有数据 |
| LIV-P6-04 | 指标与推送口径对齐验收 | Q | 2 | 02 | 对比记录 | 同一计数器两处一致 |

### P7 全产品必选化（18 人日，第 8 到 9 周）

| ID | 任务 | 角色 | 人日 | 依赖 | 交付物 | 出口证据 |
| --- | --- | --- | --- | --- | --- | --- |
| LIV-P7-01 | `product.yaml` 增加 `livePanels` 与 schema 校验 | F | 3 | P3-02 | 清单字段与校验 | 缺字段构建失败 |
| LIV-P7-02 | 通用 `LiveDynamics.vue`（清单驱动的卡片、状态、吞吐、时延） | F | 4 | 01 | 通用页面 | 三个产品接入截图 |
| LIV-P7-03 | IEMS 事件源与面板 | D+F | 3 | P2-01、02 | 采集、控制、应用事件 | 面板截图与事件计数 |
| LIV-P7-04 | SmartBox 事件源与面板 | D+F | 3 | P2-01、02 | 通道、脱扣、授权事件 | 面板截图与事件计数 |
| LIV-P7-05 | `check_live_coverage.py` 守卫与正反夹具 | C | 3 | 01、02 | 守卫脚本与夹具 | `run_guard_selftest.py` 收录 |
| LIV-P7-06 | 文档：必选要求、接入步骤、指标命名规范 | A | 2 | 全部 | 接入文档与命名表 | 评审通过 |

合计约 84 人日，9 周。P1 与 P2 可部分并行，P3 可与 P2 并行。

## 7. 派活与跟踪

### 7.1 一任务一 issue 一 PR

- issue 标题：`[LIV-P1-02] adapters/live_feed SSE 组件`。
- 标签：阶段 `P1`，组件 `component:live-feed` 或 `web:core`，类型 `type:feature|refactor|test|docs`，triage `needs-triage`，评审后换 `ready-for-agent` 或 `ready-for-human`。
- 一个任务一个 PR，重构、缺陷、功能不混（仓库规则）。
- 里程碑按 P0 到 P5 建。

### 7.2 完成定义

- C 侧：单元测试、组件门禁、SSE 契约测试（真实 socket）。
- 前端：单元测试、Playwright 场景、截图证据。
- 数据一致性：断流后重连能恢复（resync 生效），事件丢弃有计数。
- 必选性：任一产品缺 `livePanels`、事件源注册或指标注册时，`check_live_coverage.py` 与清单校验必须失败。
- 新产品或新功能合入时，同一 PR 内必须带面板、事件源与指标，缺一不算完成。
- 性能任务给数字，不给「感觉流畅」。

### 7.3 每周节奏

周一认领并确认依赖已合并；周五交证据链接，更新 §7.4 的表；阻塞超过一天升级。

### 7.4 进度总览（模板）

| 阶段 | 任务数 | 完成 | 进行中 | 阻塞 | 里程碑判据 |
| --- | --- | --- | --- | --- | --- |
| P0 | 3 | 0 | 0 | 0 | 契约、事件源、指标冻结 |
| P1 | 4 | 0 | 0 | 0 | 适配器契约测试绿 |
| P2 | 4 | 0 | 0 | 0 | 事件与快照可用 |
| P3 | 4 | 0 | 0 | 0 | 前端三种传输可切 |
| P4 | 5 | 0 | 0 | 0 | 页面四态与降级可见 |
| P5 | 3 | 0 | 0 | 0 | E2E 与性能结论 |
| P6 | 4 | 0 | 0 | 0 | 指标端点与看板可用 |
| P7 | 6 | 0 | 0 | 0 | 全产品面板与守卫绿 |

## 8. 验证与验收

- SSE 契约：用测试客户端连真实端口，验证订阅过滤、合并、队列满丢弃、保活注释、断线清理。
- 数据一致性：先取快照，再注入事件，模拟丢包触发 resync，页面回到正确状态。
- E2E：用 `acceptance/dcu` 的启动壳拉起产品，Playwright 打开动态图，四条用例对应四种状态与断流。
- 性能：1000 表仿真，记录事件率、浏览器主线程占用、页面帧率；不达标先合并再优化时，结论写清。
- 指标：抓取 `/api/v1/system/metrics` 两次，校验文本格式与计数变化；用同一仿真核对推送与指标两处口径。
- 合并后跑 `run_guard_selftest.py` 与前端测试；后端新组件进 CI 矩阵。

## 9. 风险与开放问题

需人拍板：

1. 传输选 SSE 还是 WebSocket。本方案首版 SSE，理由见 §5.1。
2. 动态图的默认视图：网格还是拓扑。拓扑需要台区拓扑数据，DCU 现有 `DistributionArea` 有分支结构，但页面是否要画线需要产品定。
3. 事件保留：是否需要在服务端留短期回放（当前契约明确不保留，靠 resync 重取）。
4. 鉴权：沿用现在的无鉴权模型，还是本任务顺带加一层（会影响 REST 与实时通道两边）。
5. 事件率上限目标：1000 表每分钟一次之外，是否有更密集的采样需求。
6. 指标交付范围：只给 Prometheus 端点，还是连 Grafana 看板与 docker 示例一起交付；指标保留与抓取周期由谁定。

技术风险：

- `rest_server` 没有流式响应能力，实时通道必须新起监听或改造，二者都要处理端口与防火墙。
- 前端 store 的 resync 语义依赖消费者重新取数；新页面必须实现，否则断档会显示旧状态。
- `AppShell` 现在无条件连接，修复顺序要在服务端上线之前，避免控制台持续报错掩盖问题。
- 动态图在高刷新率下容易做成 CPU 黑洞，合并与降帧是硬要求，验收带数字。

## Appendix A. Links and reading list

- `web/core/src/sdk/live.ts`、`web/core/src/app/stores/live.ts`（前端实时契约）
- `docs/design/edgeos_web_ui_ux_masterplan.md`（§3 可视化与 §4.1 流式渲染）
- `adapters/rest_api/src/rest_server.c`（REST 路由与限制）
- `cmd/dcu/dcu_poller.c`、`cmd/dcu/dcu_storage.c`（抄表与最新值表）
- `web/products/dcu/product.yaml`、`web/products/dcu/pages/MetersView.vue`（产品前端装配）
- `docs/adr/0001-observability-metrics-channel.md`（指标通道 ADR）
- `modules/edge_core/include/edge_metrics.h`（指标注册与上报 API）

## Appendix B. Prototype evidence

本轮没有跑原型。传输选型靠实现量与语义对比得出（§5.1），若评审要求实测，P0 可补一个最小 SSE 与 WebSocket 的对比原型，记录分支与 SHA。
