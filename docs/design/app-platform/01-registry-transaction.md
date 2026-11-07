# 01 贡献注册表的事务模型

状态：设计基线，待评审。未实现，未做故障注入。

## 1. 要解决的问题

一次安装会同时影响点位表、UI 路由、权限点、北向映射、告警规则。任何一处成功、另一处失败，都会留下"页面在但点位不在"这类中间态。升级和卸载同样要跨这些组件。

本文件给出：谁是写者、状态怎么变、事务边界在哪、每个可能的中断点失败后是什么状态、怎么恢复。

## 2. 写者与读者

**单一写者。** 只有 `app-manager` 进程写注册表。它持有 SQLite 的 `BEGIN IMMEDIATE`，同一时刻只处理一个事务。并发安装请求排队，超时返回 `busy`，不做并行合并。

**读者不碰 SQLite。** 每次提交后，`app-manager` 生成一份不可变快照文件 `/run/edgeos/registry/gen-<N>.json`，先写临时文件，`fsync`，再 `rename` 成最终名，最后更新指针 `/run/edgeos/registry/current`（内容是一个数字，同样原子 rename）。

- DataHub、北向、UI、告警、调度各自读 `current` 指向的快照，只读，不加锁。
- 读者缓存必须按 generation 标记。generation 变化即失效。
- 快照是不可变的，读者不需要担心读到半份数据。

这样把"跨组件一致"降成了"一个文件指针的原子替换"。SQLite 事务只保证注册表自身，快照发布保证读者看到的是同一代。

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
  capability    TEXT
  scope         TEXT            -- JSON，点位或主题范围
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
| 1 validate | 验签、schema、sdk、能力、配额、冲突 | 无写入 | 什么都没发生 |
| 2 stage | 写 `app_version(staged)` 与 `contribution(staged)` | 单个 SQLite 事务 | 整步回滚，删除该 txn 的 staged 行 |
| 3 commit | 把该 txn 的 staged 行改 `active`，写 `app_version(committed)`，generation+1，写 txn_step(commit) | 单个 SQLite 事务 | SQLite 回滚；staged 行留待启动清理 |
| 4 publish | 生成快照 `gen-N.json`，原子替换 current | 文件系统原子 rename | 重试；失败不改变注册表，读者仍看旧代 |
| 5 activate | 启动运行时，健康检查 | 不在事务内 | 转 `rollback_pending`，走补偿 |
| 6 finalize | `app.state=active`，txn done | 单个 SQLite 事务 | 重启后由对账修复 |

第 3 步和第 5 步之间**不可能原子**。运行时启动是外部动作，数据库管不到。所以这里必须是 saga，不能宣称两阶段提交。文档和评审口径都要这么写。

## 6. 中断点与恢复矩阵

| 中断位置 | 掉电后注册表状态 | 启动时动作 | 对用户可见的结果 |
| --- | --- | --- | --- |
| 步骤 1 中 | 无变化 | 无 | 未安装 |
| 步骤 2 中 | SQLite 回滚，无 staged 行 | 无 | 未安装 |
| 步骤 2 后、3 前 | 有 `staged` 行，txn open | 启动扫描：删除超过 60 秒的 staged 行，txn 标 aborted | 未安装 |
| 步骤 3 中 | SQLite 回滚 | 同上一行 | 未安装 |
| 步骤 3 后、4 前 | 注册表已 active，快照仍是旧代 | 启动扫描：generation 比 current 大则重新发布快照 | 应用已装，读者短暂看旧数据 |
| 步骤 4 后、5 前 | 注册表与快照都是新代，运行时未起 | 启动对账：`app.state=installed` 且有 active 贡献，则尝试启动 | 安装完成，启动稍晚 |
| 步骤 5 中 | `activating` | 启动对账：重试启动一次，失败转 `rollback_pending` | 失败则回到上一版本或未安装 |
| 步骤 6 前 | 运行时已健康，`activating` | 启动对账：补写 `active` | 正常 |
| 补偿中 | `compensating` | 重放补偿步骤，幂等 | 最终成为 retired |

启动对账是唯一允许"猜"的地方，规则固定：`activating` 一律重试一次；仍失败则补偿；`compensating` 一律重放；`open` 且超过 TTL 一律 abort。

## 7. 补偿不是回滚代次

补偿只做一件事：把新版本的贡献标 `retired`，把 `app.active_version` 指回上一版本（如果有），然后推进 generation。

不做的三件事：不删除行、不回退 generation、不撤销已经发布的快照。读者可能已经基于新代做了一次读取，回退代次会让读者状态倒退，比多一次前向提交更难推理。

## 8. 升级与回滚

升级：

1. 新版本以新的 `app_version` 行写入，贡献以新 `app_version` 暂存。
2. 提交时 `active_version` 指向新版本，generation+1。
3. 激活健康检查通过后，旧版本行标 `retired`，但保留一个版本用于回滚。
4. 旧版本的 grant 不被自动继承。新版本必须重新列出能力，管理员批准沿用旧决定时也要写一条新记录，注明 `inherited_from`。

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

点位不删除。DataHub 保留 tombstone 元数据，`lifecycle=retired`。历史曲线仍能解析点位名，北向停止上送，写入一律拒绝。UI 路由从快照中消失。

删点位会让历史查询出现空洞，所以明确不删。

## 10. 并发

- 安装、升级、卸载、回滚全部串行，由 app-manager 单线程处理。
- 读者不受影响，读的是不可变快照。
- 同一应用的两个安装请求，第二个在拿到锁后检查 `state`，发现目标版本已存在则返回 `already_installed`，不做幂等重复。
- 跨应用并发也串行。收益是推理简单，代价是安装吞吐低。现场场景可接受。

## 11. 验收

每条都要有可复现的测试，不是描述。

1. 步骤 2 注入失败，注册表无任何残留。
2. 步骤 3 注入失败，无 active 贡献，无新 generation。
3. 步骤 3 后 kill -9，重启后快照重新发布，读者看到新代。
4. 步骤 5 注入启动失败，应用回到 `rollback_pending`，补偿后旧版本仍可用。
5. 补偿执行到一半 kill -9，重启后重放完成，结果一致。
6. 升级后回滚，贡献集合与回滚前逐条相等。
7. 卸载后历史查询仍能解析点位名，写点位被拒绝。
8. 两个并发安装同一应用，一个成功，一个得到 `already_installed`。
9. 快照文件在写入一半时断电，current 仍指向旧代，新文件启动时被清理。
10. 启动对账在 12 种中断点组合上各跑一次，最终状态与预期一致。

## 12. 未决

- 快照文件大小上限。应用很多时，全量快照可能过大，是否需要分片或按查询建索引。
- 启动对账的时间预算。设备上应用很多时，逐个重试可能拖慢开机。
- grant 的继承语义是否应该默认拒绝。当前写的是"必须重新批准"，需要确认现场能否接受。
- 历史点位的 tombstone 保留多久，是否会无限增长。
