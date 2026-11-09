# 05 P0 最小可验证切片

状态：设计基线，待评审。未实现。

## 1. 目的

这不是演示，是证伪。P0 只验证三个最关键的假设：

1. 注册表事务模型在真实故障下是否自洽。
2. 能力申请与授权链路能否拦住越权，并且撤销立即生效。
3. WASM 单实例故障是否真的不扩散，宿主崩溃后能否恢复。

三个假设都成立，才允许扩展贡献类型和语言。

## 2. 范围

分两层，先 core 后 ext。core 只验证三个假设，ext 是满足日常使用必须但可以先缺的部分。

| 维度 | 分层 | 取值 | 明确不做 |
| --- | --- | --- | --- |
| 执行档位 | core | `execution_profile: wasm`，默认 vendored WAMR（wasmtime 仅作备选后端，见 04 号文档） | QuickJS、Go sidecar、Python |
| 语言 | core | C | C++、Rust、Go |
| 贡献类型 | core | `telemetry.points`，含 `point_publish` 只写自身命名空间 | `ui.pages`、`northbound.metrics`、`alarm.rules`、`schedule.jobs`、`protocol.handlers` |
| 能力 | core | `tags.read` 单点位、`points.publish` 自身命名空间、`log` | `tags.write`、`alarms.raise`、`net.egress` |
| 签名 | core | 一个测试密钥，真实验签 | 密钥轮换、证书链 |
| 分布 | core | 本地安装包，不经网络 | 商店后端、远程下载 |
| 参数 | ext | 清单 `settings` schema、默认值、`setting_get`、`appctl settings set` | WebUI 设置页、参数分组 |
| 事件 | ext | `settings.changed`、`grant.changed`、`upstream.available`、`upstream.unavailable` | 应用之间的事件、持久重放 |
| 界面 | ext | 无应用页面，只有一个只读的状态接口 | 应用商店前端、详情页 |

`points.publish` 进 core，是因为验收里"卸载后历史解析点位名"与"故障后点位质量"都需要应用先发布点位；没有发布，这两条无从验证。`tags.write`（写产品点位）仍然不开放。

## 3. 组件

两个常驻 + 一个命令行，全部最小实现。

1. `app-manager`：**产品进程内的模块**，不是独立进程。单写者，负责安装、批准、启动、停止、卸载、回滚，写注册表与快照，运行调谐与状态接口。
2. `app-host`：独立进程，由产品进程 `posix_spawn` 拉起并继承一对 `socketpair`；加载 WASM 实例，提供 Host ABI，做配额与故障处理。
3. `appctl`：命令行，经 unix socket 调产品进程，用于安装、批准、启停、查看状态。P0 不做 WebUI。
4. `registry-snapshot`：app-manager 内的一个函数，生成 `gen-N.json` 并原子发布，不是独立组件。

不单独实现 supervisor：app-host 的存活由产品进程负责；产品进程退出时 app-host 随 `PR_SET_PDEATHSIG` 退出。

**已定的前提（2026-11-09）：**

- **主进程**是产品进程（P0 为 `dcu`），不是 Go 网关。`go/cmd/edge-gateway` 只在 dev mesh 中存在，生产镜像不启动。
- **应用点位承载**：注册表给每个点分配槽位，产品进程计算 `app_tag = app_base + slot`；DataHub 在 `map_fn` 返回 `<0` 时回退为 `idx = tag_id`，因此值走既有影子表与 ring。除容量常量与产品 `map_fn` 的让位规则外，不改 DataHub 结构。
- **进程间通道**：继承的 `socketpair`，每实例令牌，句柄带 `grant_generation`。

**旧 `env` ABI（2026-11-09 已定）：拒绝。** 未声明 `abi: 1` 的模块安装即拒，错误码 `DENIED_ABI`，不提供兼容层。`examples/wasm/*` 用 `ext_datahub_read(1001, ...)` 加全局数字 TagID，与 ADR-0010 冲突；示例与指南标 `legacy`，P0 交付一个最小迁移示例。现场若盘出真实旧应用，再单独评审兼容层。

除上述已定路径外，P0 对现有代码的改动只允许：产品进程启动流程增加 app-manager 初始化与 app-host 拉起；产品 `map_fn` 增加 `tag_id >= app_base` 的让位；影子表容量常量与 Go 侧同步。其他子系统不动。

## 4. 接口

命令行足够验证，不做 Web 接口。

```
appctl install <pkg> --key <pub>      # 验签、校验、暂存、提交
appctl grant <app> <cap> --scope <s>  # 批准能力
appctl revoke <app> <cap>
appctl start <app>
appctl stop <app>
appctl upgrade <app> <pkg>
appctl rollback <app>
appctl uninstall <app>
appctl status [<app>]
appctl snapshot                        # 打印当前 generation 与贡献摘要
```

状态接口 `appctl status` 输出机器可读 JSON，供验收脚本断言。

## 5. 交付物

1. `app.yaml` 的 JSON Schema v1。
2. WIT 包 v1 的 P0 子集，函数名以 03 号 ABI 总表为准：`point_resolve`、`point_read`、`point_subscribe`、`point_publish`、`log`、`now`，以及导出 `app_init`、`app_on_event`、`app_stop`；`setting_get` 在 ext 层加入。不再使用 `tag_read`、`now_ms` 这类旧名。
3. C 绑定，由 WIT 生成，CI 校验不可手改。**P0 只交付 C 绑定与 C 示例应用；Rust/Go/Python 在 P1，JS 在 P2，不提前承诺。**
4. 注册表 schema 与快照格式说明。
5. 参数 schema 与 `app_settings` 存储，含默认值初始化与字段级校验。
6. 包校验器：验签、解包限制、导入白名单，作为独立模块，可单测。
7. 一个示例 C 应用：订阅一个点位，值超阈值时写日志。
8. 一个 legacy 反例：用旧 `env` ABI 编译的模块，用于验证被拒。
9. test-platform 的 `edgec-app` 任务与适配器：拉起产品进程与 app-host，执行第 6 节检查，证据落 JSONL，`gate` 判四档结论。
10. 故障注入适配器（test-platform 插件），覆盖第 01 号文档列出的中断点矩阵。
11. DCU `dcu@1` 产品剖面 v1，至少包含 `area.voltage.min/avg/max` 三个台区聚合点；聚合由 DCU 产品侧计算，应用平台只消费剖面。
12. `edgeos-sim` 开发宿主（P1）：同一 app-host/WAMR、假剖面与内存 DataHub，供 SDK 作者本地运行；不模拟授权，不进生产镜像。
13. `edgeos-pack` 打包工具（P1）：校验前置、确定性 tar.zst、Ed25519 签名，输出 `.eapp`；设备不打包。

## 6. 验收项与证据

**用例分两层（2026-11-09 已定）。** ABI conformance 放 EdgeC `tests/`（cmocka + WAMR 宿主，快、进 CI）；端到端验收放 test-platform（进程拉起、故障注入、evidence/gate，四档结论）。两边不重复：`tests/` 断言机制，test-platform 断言“装上去能用、坏了能恢复、证据可查”。

每一条都要有可复现的命令和输出，不接受"人工观察"。

| # | 检查 | 证据 |
| --- | --- | --- |
| 1 | 两份错误清单被拒：含通配符能力、未知贡献类型 | 退出码与错误码 |
| 2 | 未签名包被拒 | 退出码与错误码 |
| 3 | 只读能力未批准时，应用可安装但不可启动 | 状态 JSON 中 `state=installed`、`start` 返回拒绝 |
| 4 | 批准后可启动，经产品端口读到授权点位；应用宿主不直接引用 `datahub.h` | 应用日志、状态 JSON 与边界检查 |
| 5 | 读未授权点位返回 `denied_not_granted` | 错误码与审计记录 |
| 6 | 撤销后 200ms 内下一次读取返回 `denied_revoked` | 时间戳对比 |
| 7 | 同版本号替换包内容后启动被拒，返回 `denied_package_mismatch` | 错误码与审计记录 |
| 8 | 死循环触发中断，实例 `faulted`，同宿主其他实例仍在运行 | 两个实例的状态与日志 |
| 9 | 内存超限实例失败，宿主存活 | 状态 JSON 与宿主 pid 未变 |
| 10 | `kill -9 app-host` 后 5 秒内恢复，应用重新激活 | 时间戳与状态 JSON |
| 11 | 安装时在第 3 步与第 4 步之间**真实断电**（dm-flakey 或断电继电器，不用 `kill -9`），重启后从数据库重建快照，恢复为完整的旧代或完整的新代 | 重启前后的数据库与快照摘要对比 |
| 12 | 补偿执行到一半真实断电，重启后重放完成 | 快照摘要与 txn 日志 |
| 13 | 升级后回滚，贡献集合与升级前逐条相等；继承的 grant 生效，新增/扩大的 grant 在批准前不生效 | 快照 diff 与授权状态 |
| 14 | 卸载后产品侧历史仍能解析点位名，写入被拒 | 历史查询返回与写入错误码 |
| 15 | 两个并发安装，一个成功一个 `already_installed` | 两次退出码 |
| 16 | 手改 C 绑定，CI 失败 | CI 日志 |
| 17 | 未声明 `abi: 1` 的旧 `env` 模块被拒，返回 `DENIED_ABI` | 退出码与审计 |
| 18 | 模块声明未在白名单内的 import，加载被拒，返回 `denied_import` | 错误码与进程状态 |
| 19 | 超大解压包与深层 JSON 被拒，且未解压落盘 | 磁盘占用与退出码 |
| 20 | 有消费者时：落后显示"收敛中"，全部到达目标 generation 才显示完成；无消费者时（P0 只有 appctl）安装完成不声称"已收敛" | 状态 JSON 与各消费者 `applied_generation` |
| 21 | 手工停掉一个运行中的应用，调谐循环 30 秒内恢复 | 时间戳与状态 JSON |
| 22 | 单应用 Host 调用超预算返回 `denied_quota`，其他应用不受影响 | 错误码与相邻应用日志 |
| 23 (ext) | 安装后首次启动读到全部默认参数，无需先配置 | 应用日志与状态 JSON |
| 24 (ext) | 参数类型错误、越界、未声明的键被拒，返回字段级错误 | HTTP 或命令行错误码 |
| 25 (ext) | `secret` 字段通过接口只返回"已设置"，不返回明文 | 接口响应 |
| 26 (ext) | 升级时参数迁移演练失败，升级被阻断，旧版本仍可用 | 升级退出码与状态 |
| 27 | 安装应用 B 前后，应用 A 的包摘要、参数行、KV 行逐字节不变 | 安装前后摘要对比 |
| 28 | 磁盘写满时安装失败，现有代不受影响，错误可读 | 磁盘占用与状态 JSON |
| 29 | 墙钟发生跳变时，应用使用的单调时钟不跳，超时与间隔判断不受影响 | 应用日志中的两种时钟 |
| 30 | grant 表被外部损坏，启动拒绝服务并提示恢复，不静默放行 | 启动退出码与审计 |
| 31 | 宿主级崩溃 3 次/5 分钟进入 degraded 并停止重拉；可归因到同一实例的毒应用 2 次/10 分钟进入 `isolated` | 状态 JSON 与重启计数 |
| 32 | 日志洪泛下事件队列有界，调谐循环仍按周期运行 | 队列长度与调谐时间戳 |
| 33 | 应用故障后其点位质量变为 `stale` 或 `bad`，页面不显示为实时值 | 点位质量与页面状态 |
| 34 | `requires.points` 未被 `tags.read` 覆盖，或授权范围包含未声明的点位，安装返回 `INVALID_MANIFEST` | 退出码与错误码 |

第 11 到 13 条是 P0 的核心，第 17 到 20 条是安全与一致性的底线，第 27 条是"不改别的应用"的直接证据。

## 7. 退出条件

分两道门。

**P0-core 门**（先过）：三个假设的验收全过，证据归档；故障注入可重复；没有一项依赖人工判断；注册表在所有中断点上的最终状态与 01 号矩阵一致；目标板侧写一次 CPU、内存、安装、启动、恢复耗时。

**P0-ext 门**（core 之后）：参数、密钥脱敏、事件三类验收全过。

任何一条不满足，范围不变，先修机制，不加功能。core 未过不进 ext，ext 未过不进 P1。

## 8. 明确不在 P0 内

- 应用商店界面与安装体验。
- 任何应用界面：自动详情页、声明式页面、沙箱 webview 都不在 P0，见 07 号文档的分阶段。
- 北向自动上送。
- 告警规则。
- Go、Python、JS。
- 签名基础设施与密钥管理。
- 多设备分发、灰度、回滚编排。
- 性能指标承诺。

## 9. 风险

- 目标板上的 WASM 运行时可能不支持所选的中断机制，选型需要在第一天确认，否则整个 A 档要重做。
- 掉电注入在真实硬件上可能不可控，需要可编程电源或内核级注入，测试环境要先具备。
- 兼容性风险：DataHub 结构不改，注册表维护应用点位名字与槽位，产品 `map_fn` 对 `tag_id >= app_base` 让位；北向与历史查询继续按现有产品点位工作，需要在 P0 内做一次兼容性验证。
- 5 秒恢复目标是判断，不是测量。若实测明显更慢，需要重新评估"宿主进程崩溃可接受"这一前提。
