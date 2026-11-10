# 01 贡献注册表的事务模型

状态：第二轮修订（2026-11-09/10），待复审。未实现，未做故障注入。

## 1. 要解决的问题

一次安装会同时影响点位表、UI 路由、权限点、北向映射、告警规则。任何一处成功、另一处失败，都会留下"页面在但点位不在"这类中间态。升级和卸载同样要跨这些组件。

本文件给出：谁是写者、状态怎么变、事务边界在哪、每个可能的中断点失败后是什么状态、怎么恢复。

## 2. 写者与读者

**唯一写者。** app-manager 是产品进程内的一个模块，不是独立进程；只有它写注册表。它持有 SQLite 的 `BEGIN IMMEDIATE`，同一时刻只处理一个事务。并发安装请求排队，超时返回 `busy`，不做并行合并。管理面与授权判定同在产品进程，撤销不需要跨进程同步。

现有两处相关写入必须收敛：

- Go 网关的 `/api/v1/internal/registry` 与 `UIConfigs` 联邦只在 dev mesh 里存在，生产镜像不启动；若保留，改为只读适配器，不再写应用状态。
- `edge_app_mgr` 的 `app_registry` 表保留为兼容视图，由 app-manager 写入；其他组件只读。注意：现有 `edge_app_mgr` 没有被任何产品构造（只有测试），`start/stop` 只改数据库状态；它不能作为注册表的现状基线。

**读者不碰 SQLite。** 每次发布后，`app-manager` 生成一份不可变快照 `/run/edgeos/registry/gen-<N>.json`，先写临时文件，`fsync`，再 `rename`，最后原子更新指针 `/run/edgeos/registry/current`。`/run` 是 tmpfs，重启即丢，视为缓存；数据库是唯一持久事实源，启动时从数据库重建。

- DataHub、北向、UI、告警、调度各自读 `current` 指向的快照，只读，不加锁。
- 读者缓存按 generation 标记，generation 变化即失效。
- 原子 rename 只保证**单个读者**读到完整的一代，不保证所有读者同时切换。

**快照内容（2026-11-09 已定）。** 单文件四段投影，消费者只读自己那段，但看到的是同一代：

- `apps`：应用 id、版本、publisher_id、execution_profile、settings schema、fault policy。
- `points`：应用点位名、slot、类型、单位、contract_version、stale_after_ms、owner、retention。
- `ui`：应用声明的 pages / widgets。
- `permissions`：应用生成的核心权限点。

产品点位不进快照（产品静态剖面，构建期固定）。grant、参数值、密钥不进快照（授权与参数在控制库，只有产品进程读）。大小上限初值 4MB，每段有配额；投影超限时安装失败，不截断。保留最近两代，必需消费者都上报后才删更旧的。产品进程不读快照文件（它持有内存模型并对外提供 API）；快照是给未来的 UI/北向进程与验收脚本的只读视图。

**消费者偏移不落盘。** 每个消费者在内存里保存 `applied_generation`，向注册表心跳上报；重启后从 0 开始并整份重放当前快照。单机场景没有跨重启的增量消费需求，持久化偏移只会带来代次回退问题。

**代次单调。** generation 只在发布时推进。启动时若数据库里的 generation 小于某个消费者上报值，取 `max(库内值, 上报值)` 作为当前代，强制全量 resync，并记录一条恢复事件。不允许把代次调小。

**耐久性。** 注册表使用 ADR-0007 的 `synchronous=NORMAL`，普通安装记录掉电可能丢失最近提交。**撤销授权与状态变更审计例外，使用 `synchronous=FULL`**，落盘后才返回成功，否则掉电会把已撤销的能力复活。这是对 ADR-0007 的局部例外，需要单独记一条 ADR 附注。

**分库。** 注册表、grant 与关键审计放一个控制库；`app_kv` 与设置放独立的数据库。SQLite 单库只有一个写者，应用高频 KV 写入会占住写锁，把撤销事务排在后面。分库后控制库的写延迟不受应用数据影响。

**审计分级。** 状态变更（安装、批准、撤销、升级、卸载）同步写 FULL 审计；逐次放行与拒绝只做聚合计数与批量 NORMAL，且不在 Host 调用的同步路径上。每次放行都同步 FULL 会把读取路径拖慢并加速 eMMC 磨损。

**代次对外标识。** 设备内部用单调整数 generation；对外的缓存键与协议里使用 `(boot_epoch, seq)`，或把 `snapshot_digest` 放进缓存键。原因是掉电可能丢失最近一次发布，重新启动后同一个整数 generation 可能对应不同内容，外部缓存（浏览器、北向对端）会命中错误的一代。`boot_epoch` 每次启动变化，`seq` 在本次启动内单调。`kill -9` 不丢页缓存，验证不了这一点，必须用真实掉电。

## 3. 数据模型

```
app
  id                TEXT PK
  desired_version   TEXT          -- 提交时写，期望运行的版本
  published_version TEXT          -- 健康后写，读者可见的版本
  state             TEXT          -- installed | activating | active | rollback_pending | uninstalled
  desired_state     TEXT          -- running | stopped；管理员意图，重启后按它恢复
  updated_at        INTEGER

app_version
  app_id            TEXT
  version           TEXT
  manifest          TEXT
  manifest_hash     TEXT
  package_digest    TEXT
  signature         TEXT
  package_path      TEXT
  state             TEXT          -- staged | awaiting_grant | committed | migrating | activating | healthy | published | retired | failed
  installed_at      INTEGER
  PRIMARY KEY (app_id, version)

contribution
  id                TEXT PK
  app_id            TEXT
  app_version       TEXT
  type              TEXT
  schema_version    INTEGER
  payload           TEXT
  state             TEXT          -- staged | active | retired
  generation        INTEGER
  txn_id            TEXT

txn / txn_step      -- 同前，记录每个 saga 步骤
grant               -- 绑定 (app_id, capability, scope)；版本与包摘要只进审计，见 02 号文档
```

`active` 语义拆分后，读者只认 `app.published_version`。`desired_version` 是"应该运行什么"，`published_version` 是"已经在跑且可被消费什么"。两者不一致时由调谐循环收敛。

`desired_state` 是管理员意图：安装默认 `stopped`，`start` 写 `running`，`stop` 写 `stopped`。产品进程启动时只恢复 `desired_state=running` 且未 `isolated` 的应用；手动停止的应用不会因为一次重启被复活。实际运行态（`restarting`/`faulted`/`isolated`）仍只在内存，见 11 号文档。

**运行态不进这些表。** `restarting`、`faulted`、`isolated`、重启次数是高频状态，写进注册表会牵动单写者、快照与代次。它们留在 app-manager 内存与指标里，注册表只存期望状态与已发布状态，见 11 号文档。

## 4. 状态机

应用版本：

```
staged ──commit──> awaiting_grant ──approved──> committed ──activate──> activating
                                                                          │
                                                          health ok ──────┴── fail ──> failed
                                                                 │
                                                                 └──publish──> published
published ──upgrade/uninstall──> retired
```

- `awaiting_grant`：数据库已接受，能力待批准。调谐循环**不激活、不计失败**；旧版本继续作为 published。
- `committed`：能力已批准，等待激活。
- `healthy`：运行时启动并通过健康检查，读者仍不可见。
- `published`：`published_version` 指向它，generation 推进，快照发布，读者可见。
- `failed`：本次激活失败，等待补偿或重试。
- `migrating`：升级时在激活前跑应用提供的迁移导出；失败回到 `committed` 并阻断升级，旧版本继续 published。

**等批准不是故障。** 没有显式状态时，调谐循环会把 pending 权限当成激活失败，重试后补偿，升级会被自动撤销，管理员来不及批准。`awaiting_grant` 的超时策略是：超过配置时长（初值 24 小时）仍未批准，撤销本次提交并退役新版本，不计为故障次数，不进入 `isolated`。

应用整体状态与版本状态分开，不混用。

## 5. 一次安装的步骤与事务边界

| 步 | 做什么 | 事务边界 | 失败后 |
| --- | --- | --- | --- |
| 1 validate | 验签、schema、abi/sdk、能力、配额、命名冲突、点位环 | 无写入 | 什么都没发生 |
| 2 stage | 写 `app_version(staged)` 与 `contribution(staged)` | 单个 SQLite 事务 | 整步回滚，删除该 txn 的 staged 行 |
| 3 commit | staged 改 `awaiting_grant`，写 `app.desired_version`；旧版本继续 published | 单个 SQLite 事务 | SQLite 回滚；staged 行留待启动清理 |
| 4 approve | 管理员批准能力；grant 落 `granted`，版本转 `committed` | 单个 SQLite 事务，FULL 同步 | 保持 `awaiting_grant`；超时撤销提交 |
| 5 activate | 启动运行时，健康检查；通过后标 `healthy` | 不在事务内 | 标 `failed`，转补偿 |
| 6 publish | 写 `app.published_version`、贡献 `active`、generation+1；生成快照并切换指针 | 数据库事务加文件 rename | 数据库已提交而快照未发布时，重试发布；读者仍看旧代 |
| 7 finalize | `app.state=active`，txn done | 单个 SQLite 事务 | 重启后由对账修复 |

**读者可见的时点只在第 6 步（publish）。** 健康检查之前，无论数据库怎么写，读者都看不到新贡献。这消除了"页面已出现但应用启动失败"的中间态。

## 6. 中断点与恢复矩阵

| 中断位置 | 掉电后状态 | 启动时动作 | 对用户可见的结果 |
| --- | --- | --- | --- |
| 步骤 1 中 | 无变化 | 无 | 未安装 |
| 步骤 2 中 | SQLite 回滚 | 无 | 未安装 |
| 步骤 2 后、3 前 | `staged` 行，txn open | 删除超过 60 秒的 staged 行，txn aborted | 未安装 |
| 步骤 3 中 | SQLite 回滚 | 同上 | 未安装 |
| 步骤 3 后、4 前 | `awaiting_grant` | 不激活、不计失败；超时撤销提交 | 旧版本继续服务 |
| 步骤 4 中 | SQLite 回滚 | 保持 `awaiting_grant` | 旧版本继续服务 |
| 步骤 4 后、5 前 | `committed`，`desired≠published` | 调谐循环激活一次 | 读者看不到新应用 |
| 步骤 5 中 | `activating` | 重试激活；失败转补偿 | 读者看不到新应用 |
| 步骤 5 后、6 前 | `healthy`，未发布 | 补做发布 | 应用可用，页面稍晚出现 |
| 步骤 6 中 | 数据库已发布，快照可能旧 | 从数据库重建快照，`max(库, 上报)` 为当前代 | 读者重放后一致 |
| 步骤 7 前 | `published`，app 状态未 active | 补写 `active` | 正常 |
| 补偿中 | `compensating` | 重放补偿，幂等 | 最终 retired |

启动与调谐规则固定：`awaiting_grant` 不激活也不计失败；`committed` 或 `activating` 重试一次，仍失败则补偿；`healthy` 补发布；`compensating` 重放；`open` 超过 TTL 则 abort。恢复后允许丢失最近的安装提交，但不允许出现混代，也不允许代次回退。

### 稳态调谐

按 Kubernetes 的 level-triggered 模型工作：

- 每 30 秒一次，外加事件触发。激活与补偿由调谐器异步驱动，不占安装锁。
- 读期望：`desired_version`、`app.state`、该版本的 grant 状态。
- 读观测：运行时是否有活实例、健康检查、各消费者上报的 `applied_generation`。
- 差异动作固定且幂等：`awaiting_grant` 什么都不做；`committed` 且已授权就激活；`desired≠published` 且已健康就发布。
- 连续失败阈值分两个输入，落到同一个 `isolated` 状态：**激活失败**连续 5 次；**运行时崩溃**在 10 分钟内 5 次。两者各自计数，解除条件统一为**人工解除**，或管理员配置的冷却期结束后自动解除；默认人工解除，避免坏应用反复拉起。
- 故障连击计数持久化在独立小表（`app_id`、连续失败次数、最后故障时间，NORMAL 同步，仅在故障跃迁时写），防止整机重启清零绕过阈值。其余运行态（`restarting`、`faulted`、重启次数）留在内存，见 11 号文档。

### 锁的范围

单写者锁只覆盖 SQLite 短事务（预检、暂存、提交、发布、状态更新），**不覆盖运行时激活与健康检查**。sidecar 冷启动可能几十秒，持锁会让所有安装、卸载和撤销排队。

撤销与停止走优先通道：撤销只写控制库并推进授权代次，不等待任何 saga；正在进行的安装在下一次检查点让出。

### 消费者注册表

"安装完成 = 全部消费者追平"需要知道谁是消费者，否则一个挂掉的消费者会让安装永远停在"收敛中"。

- 消费者启动时向 app-manager 注册：名称、必需或可选、上报间隔。
- 必需消费者：UI、北向。可选消费者：诊断页面、外部只读端。
- 安装完成的判定只看必需消费者；可选消费者超时后降级为"部分收敛"并告警，不阻塞安装。
- 上报协议：`POST /api/v1/apps/consumers/{name}/heartbeat`，带 `applied_generation` 与时间戳；超过 3 个周期未上报视为失联。
- **没有消费者的部署**（P0 只有 appctl）：消费者集合为空时，"安装完成"等同于快照已发布，不得对外声称"已收敛"——收敛是对消费者而言的，没有消费者就没有这个结论。

## 7. 补偿不是回滚代次

补偿只做一件事：把 `desired_version` 指回 `published_version`（如果有），把失败版本标 `retired`，不推进 generation，不删除行，不删除已发布的旧快照。若失败版本已经 `published`（只可能发生在发布后才发现运行时不健康），则推进一个新 generation 并重新发布，仍然不回退代次。

## 8. 升级与回滚

升级：新版本提交只改 `desired_version`，进入 `awaiting_grant`；批准后激活，健康后 `published_version` 指向新版本并推进 generation；旧版本在保留一个版本后标 `retired`。grant 按 `(app_id, capability, scope)` 继承：不变或收窄自动继承，新增或扩大项单独 pending；旧版本 grant 转 `suspended` 保留，回滚时恢复。

回滚：`desired_version` 与 `published_version` 一起指回旧版本，推进一个新 generation，新版本标 `retired`。旧版本的 grant 从 `suspended` 恢复为 `granted`，不重新审批；新版本的 grant 撤销。回滚是前向提交，不是代次减一。

**回滚不是降级。** 回滚到保留版本不改变"已见最高版本"，也不受 `min_security_version` 的新增限制；只有安装一个从未见过的更低版本才需要显式降级操作。这两条规则分开写，避免回滚被防降级逻辑误拦。

## 9. 卸载与历史

停止运行时；撤销 grant；贡献标 `retired`；`app.state=uninstalled`；包在宽限期后删除。点位映射保留只读，北向停止上送，写入拒绝。删映射会让历史查询出现空洞，所以不删。

## 10. 并发

安装、升级、卸载、回滚全部串行。同一应用重复安装返回 `already_installed`。跨应用也串行，代价是吞吐低，收益是推理简单。

## 11. 验收

1. 步骤 2 注入失败，无残留。
2. 步骤 3 注入失败，`desired` 未变，无新 generation。
3. 新装应用未批准时停在 `awaiting_grant`，调谐循环不激活、不计失败；批准后才激活。
4. 升级应用未批准时旧版本继续 published；超时后新版本退役，旧版本不受影响。
5. 步骤 5 注入失败，读者从未看到新代，旧版本仍可用。
6. 步骤 5 后 kill -9，重启补发布，读者看到一致状态。
7. 步骤 6 中 kill -9，重启从数据库重建快照，代次不回退。
8. 补偿执行到一半 kill -9，重启后重放完成。
9. 升级后回滚，贡献集合逐条相等，旧版本 grant 从 `suspended` 恢复，新版本继承的 grant 失效。
10. 回滚到保留版本不被防降级逻辑拦截；安装从未见过的更低版本被拦。
11. 卸载后历史可解析点位名，写入被拒。
12. 并发安装同一应用，一个成功一个 `already_installed`。
13. 删除 `/run/edgeos/registry` 后重启，快照完整重建。
14. 必需消费者落后时状态为"收敛中"；可选消费者失联只降级为"部分收敛"并告警。
15. 一个 sidecar 冷启动 30 秒期间，撤销授权与停止命令不被阻塞。
16. **真实掉电**：用 dm-flakey 或硬件断电继电器，在步骤 3、5、6 各断电一次，结果与矩阵一致。`kill -9` 不能替代，它不丢页缓存。
17. 掉电丢失最近发布后，外部缓存键因 `boot_epoch`/`snapshot_digest` 变化而失效，不命中旧内容。
18. 磁盘写满时安装失败但不破坏现有代，错误可读。
19. grant 表被外部损坏时，启动拒绝服务并提示恢复，不静默放行。
20. `appctl` 与调谐循环并发操作同一应用，结果收敛且无重复副作用。
21. 日志洪泛下，调谐循环仍按周期运行，事件队列不无限增长。
22. 应用高频写 KV 时，撤销授权仍在 200ms 内落盘，不被数据写锁阻塞。
23. 新应用的 `requires.points` 与已装应用的 `contributes.points` 形成环，或依赖深度超过 4 时，安装返回 `DENIED_DEPENDENCY` 并指出边。

## 12. 未决

- 启动对账的时间预算。
