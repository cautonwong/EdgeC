# 01 贡献注册表的事务模型

状态：设计基线，待评审。未实现，未做故障注入。

## 1. 要解决的问题

一次安装会同时影响点位表、UI 路由、权限点、北向映射、告警规则。任何一处成功、另一处失败，都会留下"页面在但点位不在"这类中间态。升级和卸载同样要跨这些组件。

本文件给出：谁是写者、状态怎么变、事务边界在哪、每个可能的中断点失败后是什么状态、怎么恢复。

## 2. 写者与读者

**单一写者。** 只有 `app-manager` 进程写注册表。它持有 SQLite 的 `BEGIN IMMEDIATE`，同一时刻只处理一个事务。并发安装请求排队，超时返回 `busy`，不做并行合并。

**数据库是持久事实源，快照是缓存。** 每次提交后，`app-manager` 生成一份不可变快照文件 `/run/edgeos/registry/gen-<N>.json`，先写临时文件，`fsync`，再 `rename` 成最终名，最后更新指针 `/run/edgeos/registry/current`（内容是一个数字，同样原子 rename）。`/run` 下的文件可以丢失，重启后从数据库重建，重建完成前不对外提供旧代。

- DataHub、北向、UI、告警、调度各自读 `current` 指向的快照，只读，不加锁。
- 读者缓存必须按 generation 标记。generation 变化即失效。
- 快照是不可变的，读者不需要担心读到半份数据。

原子 rename 只保证**单个读者**读到完整的一代，不保证所有读者在同一时刻切换。每个消费者必须在状态里报告自己当前的 generation，装配层据此判断是否收敛；收敛前不得对外宣称安装或升级完成。

SQLite 使用 ADR-0007 规定的 `synchronous=NORMAL`。WAL 模式下掉电可能丢失最近的已提交事务，但数据库本身一致。因此本文件不承诺"已确认提交必保留"，只承诺"恢复后不是完整的提交前状态，就是完整的提交后状态，不会出现混代"。若业务要求更强的耐久性，必须单独提交 ADR，不得在应用平台文档里默认提高同步级别。

## 3. 数据模型

```
app
  id            TEXT PK
  active_version TEXT           -- 指针，不是外键，允许指向 retired 版本
  state         TEXT            -- installed | activating | active | rollback_pending | uninstalled
  updated_at    INTEGER

app_version
  app_id        TEXT
  version       TEXT
  manifest      TEXT            -- 原始 app.yaml
  manifest_hash TEXT
  signature     TEXT
  package_path  TEXT
  state         TEXT            -- staged | committed | retired
  installed_at  INTEGER
  PRIMARY KEY (app_id, version)

contribution
  id            TEXT PK
  app_id        TEXT
  app_version   TEXT
  type          TEXT            -- telemetry.points | ui.pages | ...
  schema_version INTEGER
  payload       TEXT            -- JSON，已通过 schema 校验
  state         TEXT            -- staged | active | retired
  generation    INTEGER         -- 写入时的代次
  txn_id        TEXT

txn
  id            TEXT PK
  kind          TEXT            -- install | upgrade | uninstall | rollback
  app_id        TEXT
  state         TEXT            -- open | committed | compensating | done | aborted
  started_at    INTEGER
  updated_at    INTEGER

txn_step
  txn_id        TEXT
  step          TEXT            -- validate | stage | commit | publish | activate | finalize | compensate
  state         TEXT            -- pending | done | failed
  detail        TEXT
  PRIMARY KEY (txn_id, step)

grant
  app_id        TEXT
  app_version   TEXT            -- 授权绑定版本，升级不继承
  package_digest TEXT           -- 签名包摘要，防止同版本号换包
  capability    TEXT
  scope         TEXT            -- JSON，由产品 ACL 解析，不含全局 TagID
  granted_by    TEXT
  granted_at    INTEGER
  expires_at    INTEGER
  revoked_at    INTEGER
  revoked_by    TEXT
```

`generation` 单调递增，永不回退。回滚不是把代次减一，而是提交一代新的状态。

## 4. 状态机

应用版本：

```
staged ──commit──> committed ──activate ok──> (app.active_version 指向它)
   │                    │
   └──abort──> retired  └──activate fail──> rollback_pending ──compensate──> retired
```

贡献：

```
staged ──commit──> active ──uninstall/upgrade──> retired
```

读者只认 `state=active` 且 `app_version = app.active_version` 的贡献。两个版本并存时，旧版本贡献保留为 `active` 但版本不匹配，因此对读者不可见。回滚就是切换 `active_version` 并推进 generation，不删除任何行。

## 5. 一次安装的步骤与事务边界

| 步 | 做什么 | 事务边界 | 失败后 |
| --- | --- | --- | --- |
| 1 validate | 验签、schema、abi/sdk、能力、配额、冲突 | 无写入 | 什么都没发生 |
| 2 stage | 写 `app_version(staged)` 与 `contribution(staged)` | 单个 SQLite 事务 | 整步回滚，删除该 txn 的 staged 行 |
| 3 commit | 把该 txn 的 staged 行改 `active`，写 `app_version(committed)`，generation+1，切换 `active_version` | 单个 SQLite 事务 | SQLite 回滚；staged 行留待启动清理 |
| 4 activate | 启动运行时，健康检查 | 不在事务内 | 转 `rollback_pending`，走补偿 |
| 5 publish | 生成快照 `gen-N.json`，原子替换 current | 文件系统原子 rename | 重试；失败不改变注册表，读者仍看旧代 |
| 6 finalize | `app.state=active`，txn done | 单个 SQLite 事务 | 重启后由对账修复 |

**激活可见性的选择。** 本基线采用"激活成功后才发布该代快照"：读者在应用健康之前看不到它的贡献。这样避免"页面已出现但应用启动失败"的中间态。代价是安装完成时间延后到激活之后。该选择是 P0 要验证的假设之一，若实测发现需要先发布再激活，必须同时定义失败时的可见性回退，不能只改顺序。

第 3 步和第 4 步之间**不可能原子**。运行时启动是外部动作，数据库管不到。所以这里必须是 saga，不能宣称两阶段提交。文档和评审口径都要这么写。

## 6. 中断点与恢复矩阵

| 中断位置 | 掉电后注册表状态 | 启动时动作 | 对用户可见的结果 |
| --- | --- | --- | --- |
| 步骤 1 中 | 无变化 | 无 | 未安装 |
| 步骤 2 中 | SQLite 回滚，无 staged 行 | 无 | 未安装 |
| 步骤 2 后、3 前 | 有 `staged` 行，txn open | 启动扫描：删除超过 60 秒的 staged 行，txn 标 aborted | 未安装 |
| 步骤 3 中 | SQLite 回滚 | 同上一行 | 未安装 |
| 步骤 3 后、4 前 | 数据库已提交新代，快照仍是旧代 | 启动对账：`state=committed` 的版本重试激活一次 | 读者看不到新应用 |
| 步骤 4 中 | `activating`，快照仍是旧代 | 启动对账：重试激活一次，失败转补偿 | 读者看不到新应用 |
| 步骤 4 后、5 前 | 运行时已在跑，快照仍是旧代 | 启动对账：确认运行时健康后发布快照 | 应用可用，页面稍晚出现 |
| 步骤 5 后、6 前 | 快照已是新代，`state=committed` | 启动对账：补写 `active` | 正常 |
| 步骤 6 后 | 全部完成 | 无 | 正常 |
| 补偿中 | `compensating` | 重放补偿步骤，幂等，推进新 generation 并重新发布 | 最终成为 retired |

启动对账是唯一允许"猜"的地方，规则固定：`committed` 或 `activating` 一律重试一次；仍失败则补偿；`compensating` 一律重放；`open` 且超过 TTL 一律 abort。恢复后允许丢失最近的提交（ADR-0007 的 `synchronous=NORMAL`），但不允许出现混代。

### 稳态调谐，而不只是启动对账

安装事务是边沿触发的，事件可能丢，进程可能崩，管理员可能手工改了状态。所以除了启动对账，还要有一个长期运行的调谐循环，按 Kubernetes 的 level-triggered 模型工作：

- 每 30 秒一次，外加每次事件后触发一次。
- 读期望状态：`app.state`、`active_version`、该版本的 grant。
- 读观测状态：运行时是否有活实例、健康检查结果、各消费者报告的 `applied_generation`。
- 差异动作固定且幂等：该跑没跑就启动，不该跑却在跑就停止，版本不符就切换，授权已撤销就拒绝调用。
- 同一应用连续失败时按退避重启，超过阈值转 `isolated`，不无限重试。

没有调谐循环，系统只能保证"正常情况下对"，一旦有事件丢失就停在中间态。这点比事务细节更重要，因为现场断电和手工操作都是常态。

### 消费者偏移与收敛

快照解决"读到完整的一代"，不解决"谁读到了哪一代"。每个消费者必须持久化自己的 `applied_generation`，注册表汇总出 `min_applied`。

- 应用每次应用一代的更新必须是幂等的，键是 `(generation, contribution_id)`。重复投递不产生重复状态。
- 投递语义是至少一次，不是恰好一次。恰好一次需要分布式事务，这里不引入。
- 安装或升级的"完成"定义为 `min_applied >= 目标 generation`，而不是快照文件已经 rename。
- 有消费者长时间落后时，页面显示"正在收敛"，不显示"已完成"。
- `appctl` 的写命令接受幂等键，重试同一个键返回第一次的结果，不重复执行。

## 7. 补偿不是回滚代次

补偿只做一件事：把新版本的贡献标 `retired`，把 `app.active_version` 指回上一版本（如果有），推进 generation，并发布这一代的新快照。

不做的三件事：不删除行、不回退 generation、不删除已经发布的旧快照文件。读者可能已经基于新代做过一次读取，回退代次会让读者状态倒退，比多一次前向提交更难推理。

## 8. 升级与回滚

升级：

1. 新版本以新的 `app_version` 行写入，贡献以新 `app_version` 暂存。
2. 提交时 `active_version` 指向新版本，generation+1。
3. 激活健康检查通过后发布快照，旧版本行标 `retired`，但保留一个版本用于回滚。
4. 旧版本的 grant 不被自动继承。grant 绑定 `app_version` 与包摘要，新版本必须重新列出能力；管理员批准沿用旧决定时也要写一条新记录，注明 `inherited_from`。

回滚：

1. `active_version` 指回旧版本，generation+1。
2. 新版本行标 `retired`，包保留。
3. 新版本获得的 grant 全部撤销，除非管理员显式保留。

回滚的目标是"上一版本曾经跑过的状态"，不是"重新走一遍安装"。

## 9. 卸载与历史

卸载做四件事，顺序固定：

1. 停止运行时。
2. 撤销该应用全部 grant。
3. 贡献标 `retired`，保留行。
4. `app.state=uninstalled`，包目录在宽限期后删除。

点位不删除，但保留位置取决于 P0 选定的落地路径：由产品 ACL 承载时，产品侧历史存储保留解析映射；由 DataHub 扩展承载时，DataHub 保留 tombstone 元数据，`lifecycle=retired`。两种情况下历史曲线都能解析点位名，北向停止上送，写入一律拒绝，UI 路由从快照中消失。

删映射会让历史查询出现空洞，所以明确不删。

## 10. 并发

- 安装、升级、卸载、回滚全部串行，由 app-manager 单线程处理。
- 读者不受影响，读的是不可变快照。
- 同一应用的两个安装请求，第二个在拿到锁后检查 `state`，发现目标版本已存在则返回 `already_installed`，不做幂等重复。
- 跨应用并发也串行。收益是推理简单，代价是安装吞吐低。现场场景可接受。

## 11. 验收

每条都要有可复现的测试，不是描述。

1. 步骤 2 注入失败，注册表无任何残留。
2. 步骤 3 注入失败，无 active 贡献，无新 generation。
3. 步骤 3 后 kill -9，重启后从数据库重建快照，读者看到完整的一代。
4. 步骤 4 注入启动失败，应用回到 `rollback_pending`，补偿后旧版本仍可用，且新代从未对读者可见。
5. 补偿执行到一半 kill -9，重启后重放完成，结果一致。
6. 升级后回滚，贡献集合与回滚前逐条相等。
7. 卸载后历史查询仍能解析点位名，写点位被拒绝。
8. 两个并发安装同一应用，一个成功，一个得到 `already_installed`。
9. 快照文件在写入一半时断电，current 仍指向旧代，新文件启动时被清理，并从数据库重建。
10. 启动对账在 10 个中断点组合上各跑一次，最终状态与预期一致，且不存在混代。
11. 每个消费者暴露自己的 generation；在全部消费者收敛前，装配层不得报告安装完成。
12. 删除 `/run/edgeos/registry` 后重启，快照与指针从数据库完整重建。

## 12. 未决

- 快照文件大小上限。应用很多时，全量快照可能过大，是否需要分片或按查询建索引。
- 启动对账的时间预算。设备上应用很多时，逐个重试可能拖慢开机。
- grant 的继承语义是否应该默认拒绝。当前写的是"必须重新批准"，需要确认现场能否接受。
- 历史点位的 tombstone 保留多久，是否会无限增长。
