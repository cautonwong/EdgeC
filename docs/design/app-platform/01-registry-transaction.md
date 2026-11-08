# 01 贡献注册表的事务模型

状态：设计基线，待评审。未实现，未做故障注入。

## 1. 要解决的问题

一次安装会同时影响点位表、UI 路由、权限点、北向映射、告警规则。任何一处成功、另一处失败，都会留下"页面在但点位不在"这类中间态。升级和卸载同样要跨这些组件。

本文件给出：谁是写者、状态怎么变、事务边界在哪、每个可能的中断点失败后是什么状态、怎么恢复。

## 2. 写者与读者

**唯一写者。** 只有 `app-manager` 进程写注册表。它持有 SQLite 的 `BEGIN IMMEDIATE`，同一时刻只处理一个事务。并发安装请求排队，超时返回 `busy`，不做并行合并。

现有两处相关写入必须收敛：

- Go 网关的 `/api/v1/internal/registry` 与 `UIConfigs` 联邦改为只读适配器，不再写应用状态。
- `edge_app_mgr` 的 `app_registry` 表保留为兼容视图，由 `app-manager` 写入；其他组件只读。

**读者不碰 SQLite。** 每次发布后，`app-manager` 生成一份不可变快照 `/run/edgeos/registry/gen-<N>.json`，先写临时文件，`fsync`，再 `rename`，最后原子更新指针 `/run/edgeos/registry/current`。`/run` 是 tmpfs，重启即丢，视为缓存；数据库是唯一持久事实源，启动时从数据库重建。

- DataHub、北向、UI、告警、调度各自读 `current` 指向的快照，只读，不加锁。
- 读者缓存按 generation 标记，generation 变化即失效。
- 原子 rename 只保证**单个读者**读到完整的一代，不保证所有读者同时切换。

**消费者偏移不落盘。** 每个消费者在内存里保存 `applied_generation`，向注册表心跳上报；重启后从 0 开始并整份重放当前快照。单机场景没有跨重启的增量消费需求，持久化偏移只会带来代次回退问题。

**代次单调。** generation 只在发布时推进。启动时若数据库里的 generation 小于某个消费者上报值，取 `max(库内值, 上报值)` 作为当前代，强制全量 resync，并记录一条恢复事件。不允许把代次调小。

**耐久性。** 注册表使用 ADR-0007 的 `synchronous=NORMAL`，普通安装记录掉电可能丢失最近提交。**撤销授权与审计写入例外，使用 `synchronous=FULL` 的连接，落盘后才返回成功**，否则掉电会把已撤销的能力复活。这是对 ADR-0007 的局部例外，需要单独记一条 ADR 附注。

## 3. 数据模型

```
app
  id                TEXT PK
  desired_version   TEXT          -- 提交时写，期望运行的版本
  published_version TEXT          -- 健康后写，读者可见的版本
  state             TEXT          -- installed | activating | active | rollback_pending | uninstalled
  updated_at        INTEGER

app_version
  app_id            TEXT
  version           TEXT
  manifest          TEXT
  manifest_hash     TEXT
  package_digest    TEXT
  signature         TEXT
  package_path      TEXT
  state             TEXT          -- staged | committed | activating | healthy | published | retired | failed
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
grant               -- 同前，绑定 app_version 与 package_digest
```

`active` 语义拆分后，读者只认 `app.published_version`。`desired_version` 是"应该运行什么"，`published_version` 是"已经在跑且可被消费什么"。两者不一致时由调谐循环收敛。

## 4. 状态机

应用版本：

```
staged ──commit──> committed ──activate──> activating ──health ok──> healthy
                                     │                              │
                                     └──fail──> failed              └──publish──> published
published ──upgrade/uninstall──> retired
```

- `committed`：数据库已接受，读者不可见。
- `healthy`：运行时启动并通过健康检查，读者仍不可见。
- `published`：`published_version` 指向它，generation 推进，快照发布，读者可见。
- `failed`：本次激活失败，等待补偿或重试。

应用整体状态与版本状态分开，不混用。

## 5. 一次安装的步骤与事务边界

| 步 | 做什么 | 事务边界 | 失败后 |
| --- | --- | --- | --- |
| 1 validate | 验签、schema、abi/sdk、能力、配额、冲突 | 无写入 | 什么都没发生 |
| 2 stage | 写 `app_version(staged)` 与 `contribution(staged)` | 单个 SQLite 事务 | 整步回滚，删除该 txn 的 staged 行 |
| 3 commit | staged 改 `committed`，写 `app.desired_version` | 单个 SQLite 事务 | SQLite 回滚；staged 行留待启动清理 |
| 4 activate | 启动运行时，健康检查；通过后标 `healthy` | 不在事务内 | 标 `failed`，转补偿 |
| 5 publish | 写 `app.published_version`、贡献 `active`、generation+1；生成快照并切换指针 | 数据库事务加文件 rename | 数据库已提交而快照未发布时，重试发布；读者仍看旧代 |
| 6 finalize | `app.state=active`，txn done | 单个 SQLite 事务 | 重启后由对账修复 |

**读者可见的时点只在第 5 步。** 健康检查之前，无论数据库怎么写，读者都看不到新贡献。这消除了"页面已出现但应用启动失败"的中间态。

## 6. 中断点与恢复矩阵

| 中断位置 | 掉电后状态 | 启动时动作 | 对用户可见的结果 |
| --- | --- | --- | --- |
| 步骤 1 中 | 无变化 | 无 | 未安装 |
| 步骤 2 中 | SQLite 回滚 | 无 | 未安装 |
| 步骤 2 后、3 前 | `staged` 行，txn open | 删除超过 60 秒的 staged 行，txn aborted | 未安装 |
| 步骤 3 中 | SQLite 回滚 | 同上 | 未安装 |
| 步骤 3 后、4 前 | `desired≠published`，状态 `committed` | 调谐循环重试激活一次 | 读者看不到新应用 |
| 步骤 4 中 | `activating` | 重试激活；失败转补偿 | 读者看不到新应用 |
| 步骤 4 后、5 前 | `healthy`，未发布 | 补做发布 | 应用可用，页面稍晚出现 |
| 步骤 5 中 | 数据库已发布，快照可能旧 | 从数据库重建快照，`max(库, 上报)` 为当前代 | 读者重放后一致 |
| 步骤 6 前 | `published`，app 状态未 active | 补写 `active` | 正常 |
| 补偿中 | `compensating` | 重放补偿，幂等 | 最终 retired |

启动与调谐规则固定：`committed` 或 `activating` 重试一次，仍失败则补偿；`healthy` 补发布；`compensating` 重放；`open` 超过 TTL 则 abort。恢复后允许丢失最近的安装提交，但不允许出现混代，也不允许代次回退。

### 稳态调谐

按 Kubernetes 的 level-triggered 模型工作：

- 每 30 秒一次，外加事件触发。
- 读期望：`desired_version`、`app.state`、该版本的 grant。
- 读观测：运行时是否有活实例、健康检查、各消费者上报的 `applied_generation`。
- 差异动作固定且幂等：该跑没跑就启动，不该跑却在跑就停止，`desired≠published` 就重试激活。
- 连续失败阈值：同一应用连续 5 次激活失败进入 `isolated`。恢复条件是**人工解除**，或管理员配置的冷却期结束后自动解除；默认人工解除，避免坏应用反复拉起。

## 7. 补偿不是回滚代次

补偿只做一件事：把 `desired_version` 指回 `published_version`（如果有），把失败版本标 `retired`，不推进 generation，不删除行，不删除已发布的旧快照。若失败版本已经 `published`（只可能发生在发布后才发现运行时不健康），则推进一个新 generation 并重新发布，仍然不回退代次。

## 8. 升级与回滚

升级：新版本提交只改 `desired_version`；健康后 `published_version` 指向新版本并推进 generation；旧版本在保留一个版本后标 `retired`。旧版本的 grant 不继承，新版本必须重新批准。

回滚：`desired_version` 与 `published_version` 一起指回旧版本，推进一个新 generation，新版本标 `retired`，撤销其 grant。回滚是前向提交，不是代次减一。

## 9. 卸载与历史

停止运行时；撤销 grant；贡献标 `retired`；`app.state=uninstalled`；包在宽限期后删除。点位映射保留只读，北向停止上送，写入拒绝。删映射会让历史查询出现空洞，所以不删。

## 10. 并发

安装、升级、卸载、回滚全部串行。同一应用重复安装返回 `already_installed`。跨应用也串行，代价是吞吐低，收益是推理简单。

## 11. 验收

1. 步骤 2 注入失败，无残留。
2. 步骤 3 注入失败，`desired` 未变，无新 generation。
3. 步骤 4 注入失败，读者从未看到新代，旧版本仍可用。
4. 步骤 4 后 kill -9，重启补发布，读者看到一致状态。
5. 步骤 5 中 kill -9，重启从数据库重建快照，代次不回退。
6. 补偿执行到一半 kill -9，重启后重放完成。
7. 升级后回滚，贡献集合逐条相等，新版本 grant 失效。
8. 卸载后历史可解析点位名，写入被拒。
9. 并发安装同一应用，一个成功一个 `already_installed`。
10. 删除 `/run/edgeos/registry` 后重启，快照完整重建。
11. 消费者落后时状态为"收敛中"，全部追平后才显示完成。
12. **真实掉电**：用 dm-flakey 或硬件断电继电器，在步骤 3、4、5 各断电一次，结果与矩阵一致。`kill -9` 不能替代，它不丢页缓存。
13. 磁盘写满时安装失败但不破坏现有代，错误可读。
14. grant 表被外部损坏时，启动拒绝服务并提示恢复，不静默放行。
15. `appctl` 与调谐循环并发操作同一应用，结果收敛且无重复副作用。
16. 日志洪泛下，调谐循环仍按周期运行，事件队列不无限增长。

## 12. 未决

- 快照文件大小上限与分片。
- 启动对账的时间预算。
- grant 继承是否应默认拒绝。
- `isolated` 的默认恢复策略（人工还是冷却）。
