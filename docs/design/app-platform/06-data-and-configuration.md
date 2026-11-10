# 06 应用参数、数据与跨应用取数

状态：第二轮修订（2026-11-09/10），待复审。未实现。

## 1. 先分清三种东西

"应用的数据放在哪"这个问题里有三样不同的东西，混在一起就答不清楚。

| 种类 | 例子 | 归属 | 可见性 |
| --- | --- | --- | --- |
| 参数配置 | 越限阈值、工作模式、上报周期 | 应用私有，管理员可改 | 应用可读写，管理员可配，密钥只写不读 |
| 运行时数据 | 中间状态、缓存、游标 | 应用私有 | 只有应用自己能读写 |
| 对外发布 | 派生点位、告警、页面 | 应用声明，平台承载 | 授权后可被其他应用、北向、WebUI 消费 |

参数与运行时数据是私有的。对外发布是唯一的公共接口。跨应用取数只能走第三条，不能去翻别人的私有数据。这一条决定了"装一个新应用要不要改别的应用"的答案：不需要。

## 2. 存储位置与命名空间

应用包目录只放代码和清单，安装时校验后只读，升级整包替换。**任何运行期数据都不写在包目录里**，否则升级会覆盖。

数据全部放在设备数据区，按应用命名空间隔离：

```
/var/edgeos/data/apps/<app-id>/          # 大文件，受配额限制
  logs/                                  # 仅 diagnostics.persistLogs=true 时存在，1MB×2 轮转
控制库（SQLite，ADR-0007；注册表与关键审计同库）
  app_registry       应用清单与状态（已有表，扩展字段）
  app_settings       参数值，按 app_id + key
  app_secret         密钥密文，按 app_id + key
  app_data_meta      每个应用的 dataVersion、settingsVersion
数据库（SQLite，独立文件）
  app_kv             运行时键值，按 app_id + key
  app_point_history  应用点位历史，按 (app_id, point_name, contract_version, ts)
```

**应用点位历史（2026-11-09 已定）。** 历史不按 DataHub 槽位存，因为槽位在卸载后要回收，回收后新应用的历史会与旧数据撞车。历史键是 `(app_id, point_name, contract_version)`，放在独立的 `app_point_history` 表，属于应用数据库，与控制库分开。卸载后历史保留但查询标记"已卸载"；重装同名同版本继续，升 `contract_version` 则开新序列。写入由宿主在 `point_publish` 时完成，应用不能直接写历史。**保留与配额**：默认 raw 7 天、1m 90 天、15m 2 年（v1 只实现 raw + 1m）；每应用 128MB、全局应用历史 1GB；超配额先删最旧 raw，再删最旧 1m，删除写审计，不静默丢保留期内的 raw。宿主每 1 秒或每 200 条批量提交，`synchronous=NORMAL`，掉电最多丢最近 1 秒；降采样由产品进程低优先级线程按小时跑，1m 聚合取 last/good，不做平均，避免把越限抹平。保留期与配额可按产品剖面覆盖，Edge-Ops 只能收紧。

**为什么分库。** SQLite 单库只有一个写者。应用高频写 `app_kv` 会占住写锁，撤销授权与安装事务排在后面，违反撤销 200ms 生效的目标。分库后控制库的写延迟不受应用数据影响，审计也能按 01 号文档分级。

表结构：

```sql
app_settings(
  app_id TEXT, key TEXT, value_json TEXT,
  schema_version INTEGER, updated_at INTEGER, updated_by TEXT,
  PRIMARY KEY(app_id, key)
);

app_secret(
  app_id TEXT, key TEXT, ciphertext BLOB, nonce BLOB,
  updated_at INTEGER, updated_by TEXT,
  PRIMARY KEY(app_id, key)
);

app_kv(
  app_id TEXT, key TEXT, value BLOB, persist INTEGER DEFAULT 1,
  updated_at INTEGER,
  PRIMARY KEY(app_id, key)
);

app_data_meta(
  app_id TEXT PRIMARY KEY,
  data_version INTEGER, settings_version INTEGER,
  updated_at INTEGER
);
```

`app_registry` 在迁移期作为兼容视图由 app-manager 写入，需要补 `package_digest`、`abi`、`settings_schema_version`、`data_version` 四列；新注册表以 01 号文档的 `app` / `app_version` / `contribution` 为准。它现在存的是旧清单字段（`wasm_path`、`trigger_tag`），新平台不沿用旧语义，迁移时一次性转换。

**不复用两张旧表。** `system_params` 是编译期元数据同步到数据库的全局参数，`edge_settings` 是按产品加点位存的旧配置。两者都是全局或产品级，不能承载按应用隔离的数据，混用会让命名空间失效。

## 3. 参数配置

参数在清单里声明，不是代码里硬编码，也不是自由格式的键值对。

```yaml
settings:
  schemaVersion: 1
  fields:
    threshold:
      type: number
      title: 越限阈值
      unit: V
      default: 240
      minimum: 180
      maximum: 300
    mode:
      type: string
      enum: [warn, trip]
      default: warn
      title: 处理方式
    sampleIntervalMs:
      type: integer
      default: 1000
      minimum: 100
      maximum: 60000
      requiresRestart: false
    apiKey:
      type: string
      secret: true
      title: 平台接入密钥
```

规则：

- 类型只有 string、number、integer、boolean、array<string>；`enum` 是约束关键字，不是类型。够用即可，不引入任意 JSON。
- 每个字段必须有 `default`。安装时按默认值初始化，应用首次启动就能读到完整配置，不需要"先配置再启动"。
- `minimum`、`maximum`、`maxLength`、`enum` 是硬校验，写入不通过返回 422 与字段错误，不做静默截断。
- `secret: true` 的字段进 `app_secret`，接口永不返回明文，只返回"已设置/未设置"。应用通过 Host ABI 读取明文，只在内存中使用。

**schema 子集（2026-11-09 已定）。** 只支持 `type`（string / number / integer / boolean / array<string>，数组限长）、`title`、`description`、`default`、`minimum`、`maximum`、`minLength`、`maxLength`、`enum`、`secret`、`unit`、`group`、`order`、`readonly`、`requiresRestart`。出现 `$ref`、`oneOf`、`allOf`、`patternProperties`、`pattern` 等一律 `INVALID_MANIFEST`，不静默忽略；不做组合、条件或远程引用。`default` 必须通过同一套校验；非必填且无 `default` 的字段安装时写缺省，`setting_get` 返回 `NOT_SET`。

**secret 的存储与密钥（2026-11-09 已定）。**

1. **每设备一把主密钥**，放在 `/var/edgeos/keys/device.key`（0600，root-only）；有 TPM/secure element 时用硬件密钥，没有时用文件并显式标注"软件保护"。
2. 每个 secret 用 AES-256-GCM 加密，AAD 绑定 `app_id + key`，防止密文在应用之间搬移。
3. **备份不导出设备主密钥**；恢复到另一台设备时标记"需重新录入"，不阻断应用启动。
4. **轮换**：设备主密钥轮换时后台重加密所有 secret，完成后删旧钥；轮换期间两把都可用。
5. 没有硬件根时，文档与能力审批界面写明"软件保护，拥有 root 的攻击者可解密"，不声称等同 iOS Keychain。
- `requiresRestart: true` 的字段变更后重启应用；其余字段通过变更通知让应用热更新。
- 未声明的键写入一律拒绝。保留未知键会让配置在升级后悄悄失效。
- 参数属于应用，不属于某个版本。升级保留参数，卸载在宽限期后删除，恢复安装时可选择恢复。

**卸载宽限期（2026-11-09 已定）。** 宽限期 7 天，从卸载时刻算；期间应用不可启动，但包、参数、KV、secret 保留，重装同一 `publisher_id` + `app_id` 可原样恢复。到期删除包文件、参数、KV、secret、日志、tmpfs home 与 uid 映射；历史按保留期保留，审计永久保留；点位贡献标 `retired`，槽位回收。管理员可用 `appctl purge <app>` 跳过宽限期立即清除，需 `app.approve_write` + 双人复核；宽限期内的数据仍计入应用存储配额，配额满时提示提前 purge，不自动删。

写入路径：

1. WebUI 或 `appctl settings set` 提交。
2. 校验权限 `app.<id>:configure`。
3. 按清单 schema 校验类型与范围，失败返回字段级错误。
4. 单事务写入，写审计（谁、哪个键、旧值摘要、新值摘要；密钥只记"已变更"）。
5. 通知运行中的实例；需要重启的按故障策略重启。

WebUI 的设置页由 schema 生成，不改前端代码。这是"装一个应用不用改别的应用"的另一半，页面和表单也不用手写。应用如果需要自定义界面，走第 07 号文档的三层模型，参数仍然只存这里，不随界面方案变化。

## 4. 运行时数据

- `kv` 按应用命名空间隔离，键是应用自己的字符串，不跨应用可见。
- 单值上限与总量配额由清单 `limits.storageKb` 约束，超限写失败并告警，不淘汰旧数据。
- **Python venv 配额（2026-11-09 已定）**：每个 Python 应用一个独立 venv，磁盘配额 256MB（含 venv 与包缓存）；venv 不计入 `storageKb` 的 KV 配额，单独计量；超限拒绝安装/升级，不静默删旧 venv。
- `persist` 标记决定备份与恢复：`persist=1` 进备份，`persist=0` 视为缓存，重启或恢复后可以丢。
- **并发语义（2026-11-09 已定）**：v1 只有 `kv_get` / `kv_set` / `kv_delete`，单键写入原子；不提供 CAS、TTL、批量事务或遍历。同一应用内并发写同一键，最后写入胜出，宿主不保证读-改-写原子；需要计数器的应用自己串行化，或改用点位发布。未来只以次版本新增 `kv_cas(key, expected, new)`，不改变现有语义。
- **v1 不向应用开放大文件。** KV 单值上限（初值 64KB）就是应用能存的最大块；`files/` 只给宿主（日志、备份临时文件）用，应用没有文件句柄。WASI 只开 clock/random，不提供文件访问。未来需要时只考虑宿主 blob API（`blob_open/read/write/seek/close`，应用命名空间 + 配额），不给 WASI preopen。
- 时间序列不进 KV。需要历史的数据必须以点位发布，由宿主的 `app_point_history` 负责保留与降采样；键是 `(app_id, point_name, contract_version)`，不是 DataHub 槽位。
- **写速率上限**：每个应用有每秒写次数与每秒字节数上限，以及每日写入总量上限。超限写失败并告警，不排队。理由是 eMMC 的擦写寿命，不是吞吐。批量写入由宿主合并后落盘，应用不能要求"每次写都立即 fsync"。

## 5. 升级、降级与迁移

- 参数与数据都带版本：`settings_version`、`data_version`。
- 升级时先跑迁移演练：用现有参数与数据在内存里跑 `app_migrate_settings` 与 `app_migrate_data`，任何一条失败就不允许升级。迁移函数由应用提供（ABI 可选导出，见 03 号文档），是纯函数，可单测；没有导出时只允许加法变更。
- 迁移幂等，允许重跑。迁移不删除旧字段，先写新字段。
- 降级有硬规则：如果设备上的 `data_version` 高于目标版本支持的版本，拒绝降级并给出可读原因。降级不能让旧代码读新格式的数据。
- 参数在所有版本间共享，不随版本复制。回滚不恢复旧参数，除非管理员显式选择"回滚含配置"。

## 6. 备份与恢复

- 备份包含：`app_settings`、`persist=1` 的 `app_kv`、`app_secret` 密文、`app_data_meta`、包清单与摘要。
- 备份不包含：包文件本体（可从商店重装）、`persist=0` 的缓存、运行中状态。
- 恢复到另一台设备时，密钥用设备密钥加密，可能无法解密。此时标记"密钥需重新录入"，不阻断应用启动，页面提示管理员补录。
- 恢复顺序：安装对应版本的应用，恢复参数，恢复数据，再启动。版本不匹配时先拒绝恢复该应用，记录待办，不让它带着错版本的数据跑起来。

**备份格式与一致性（2026-11-09 已定）。**

1. **归档固定为 tar.zst**，成员为 `manifest.json`（备份格式版本、设备型号、app 列表、generation、创建时间）、每个应用的 `settings.json`、`kv.jsonl`（只含 `persist=1`）、`secret.bin`（设备主密钥加密的密文，原样）、`package.digest`；成员顺序与 `mtime` 归一，归档可复现；恢复时按固定成员名校验，拒绝路径穿越。历史数据默认不进备份，需要时单独导出。
2. **一致性**：备份前不停止应用；用 SQLite `VACUUM INTO`（≥3.27）或 `sqlite3_backup` API 拿一致快照，再读文件；KV 写入在快照后短暂冻结，保证不撕裂。
3. **加密**：归档整体 AES-GCM 加密。本地滚动备份用设备主密钥派生；导出到 U 盘或云端必须用管理员口令（Argon2id 派生）再包一层，否则配置与 KV 是明文。设备主密钥永不导出。
4. **恢复**：校验归档口令/签名 → 检查 `data_version` 不高于目标版本 → 校验成员清单与包摘要 → 安装对应版本 → 恢复参数与 KV → 密钥标记"需重新录入" → 启动。任何一项失败整体拒绝，不半恢复。
5. **审计**：备份与恢复都写审计（谁、何时、包含哪些应用）。

## 7. 跨应用取数：点位是唯一公共接口

一个应用要拿另一个应用的数据，只有一条路：对方把数据以点位发布，自己申请读取授权。没有直接调用，没有共享内存，没有读对方 KV。

**结构化数据（2026-11-10 已定）。** v1 不引入 `datasets`；费率表、分时电价这类数据属于产品配置，应由产品剖面暴露为 `product.dcu.tariff.*` 点位或参数，不塞进每个应用。跨应用共享仍只走数值点位；P2 再评审只读、带 schema 与版本的 `datasets`，消费方申请 `datasets.read`，不支持任意 SQL 或写入。KV 是应用私有，不能冒充共享表。

点位的契约在清单里声明：

```yaml
contributes:
  telemetry.points:
    - name: sag_count            # 完整名 app.voltage_guard.sag_count
      type: number
      unit: 次
      contractVersion: 1         # 类型、单位、语义的契约版本
      retention: latest          # latest | latest+history
      staleAfterMs: 5000         # 超过这个时间没有新样本，读者判 stale
      stability: stable          # stable | experimental
      source: derived            # measured | derived，便于区分实测与计算值
```

消费方在清单里声明自己要什么，以及期望类型：

```yaml
requires:
  points:
    - name: app.voltage_guard.sag_count
      type: number
    - name: app.demand_forecast.next_peak_kw
      type: number
```

规则：

- 名称、类型与 `contractVersion` 必须完全匹配，v1 不做隐式类型转换。类型不符拒绝订阅，返回 `denied_type_mismatch`，而不是给一个看起来像数字的字符串。
- **声明必须自洽**：非可选 `requires.points` 必须被 `tags.read` 覆盖，`contributes.telemetry.points` 必须被 `points.publish` 覆盖；授权范围里出现未声明的点位同样拒绝。安装期校验，错误码 `INVALID_MANIFEST`。契约决定兼容性，授权决定能不能读，两者不互相推导。
- `stability: experimental` 的点位可以被消费，但安装时提示"上游可能变化"。
- 授权仍然走第 02 号文档：消费方申请 `tags.read`，管理员批准 `app.<publisher>.<point>` 范围。
- 点位的 `retention` 决定消费方能否读历史。只声明 `latest` 的点位只能拿当前值。
- `staleAfterMs` 缺省为 5 秒，允许范围 100ms–1h。`stale` 由读侧按 `server_ts` 计算，应用不能自行设置；`source_ts` 只用于展示与追溯，不参与 stale 判定。
- **双时间戳的可见性**：`point_read` 返回 `source_ts` 与 `server_ts`；只读 DataHub 影子表的进程只看到 `server_ts` 与质量。`PipeMsg` 不扩字段。
- **数据契约不是运行依赖。** 消费方可以在发布方缺席时启动并处于 `degraded`，这是数据契约；第 04 号文档禁止的是"运行依赖"，即启动顺序、互相等待、共享进程。两者不冲突。
- **禁止点位环。** 自订阅（在自己的点位上 read/subscribe）直接返回 `DENIED_SCOPE`；发布自己的点位用 `point_resolve` 返回的只写句柄，不受此限制。跨应用环在安装时静态检测：把新应用的 `requires.points` 与已装应用的 `contributes.points` 组成有向图，成环或依赖深度超过 4 则返回 `DENIED_DEPENDENCY` 并指出边；运行期不再检测。
- 发布方升级时若改单位、类型或语义，必须提升 `contractVersion`。旧版本消费方在升级时收到提示，未重新批准的读取返回 `denied_contract_changed`，不静默换算。
- 卸载发布方之前做影响分析：列出仍在消费该应用点位的应用，提示管理员；卸载后消费方转 `degraded`，不会崩溃。

### 后装应用怎么拿到已有数据

这是本节的核心问题。答案分四种情况，都不需要改发布方。

1. **当前值**。应用点位发布后由 DataHub 承载：注册表分配槽位，产品进程计算 `app_tag`，`map_fn` 返回 `<0` 时 DataHub 回退为 `idx = tag_id`。新应用安装并授权后订阅，立刻拿到当前值，不需要等下一次发布。产品点位仍走产品自己的影子表与端口。
2. **历史值**。发布方声明了 `retention: latest+history`，消费方又获批历史权限，才能回填。否则只能从现在开始积累。
3. **发布方还没装**。允许安装，应用以 `degraded` 状态启动，读取返回 `unavailable`，页面上明确显示"等待上游应用"。发布方装上并开始发布后，平台投递 `upstream.available`，消费方自动转为正常，不需要重启。安装顺序不构成依赖。事件语义见 08 号文档。
4. **发布方没有发布这个点位**。消费方拿不到，这是封装，不是缺陷。解决办法是发布方发一个新版本把点位暴露出来，这是正常的应用更新，不是"装 B 要改 A"。

### 反向同理

先装的应用要拿后装应用的数据，走同一套：后装应用发布点位，先装应用在清单里声明 `requires.points` 并获批，运行时自动订阅。发布方不知道谁在消费，消费方也不需要发布方为它做任何改动。

### 明确不做的

- 应用之间直接 RPC 调用：会把两个应用的版本和生命周期绑死。
- 读对方的 KV 或配置文件：绕过授权与封装。
- 共享内存直连：绕过类型与权限检查。
- 应用间写操作、共享状态与直接 RPC：v1 禁止；只读点位依赖允许，但必须显式声明 `requires.points`、显式授权，深度 ≤4 且禁环，见 §7。

## 8. 安装一个应用到底改了哪些行

为了回答"是不是得改别的应用"，把一次安装的写入面列清楚。

```
新增或修改：
  app_registry        一行，本应用
  app_version         一行，本应用
  app_settings        若干行，本应用默认值
  app_secret          零行
  app_kv              零行
  contribution        本应用声明的若干行
  grant               pending 或已批准的若干行，本应用

不修改：
  其他应用的任何行
  其他应用的包文件
  其他应用的参数与数据
  全局 system_params
  产品 edge_settings
```

安装只写自己的行。发布方的包摘要、参数、数据在安装前后逐字节不变，这可以作为一条验收。

## 9. 反模式

- 把参数写进包目录。升级整包替换会丢配置。
- 把应用参数塞进 `system_params`。那是编译期元数据的镜像，全局可见。
- 把应用数据塞进 `edge_settings`。那是按产品加点位存的旧模型，没有应用命名空间。
- 让应用 A 直接读应用 B 的 KV。授权、审计、版本隔离全部失效。
- 用安装顺序表达依赖。谁先装谁后装都会出现，必须让缺席成为可处理的状态。
- 用 KV 存时间序列。历史、降采样、保留策略都不归应用管。

## 10. 验收

1. 安装后应用首次启动能读到全部默认参数，不需要先配置。
2. 写入类型错误、越界、未声明的键，返回 422 与字段级错误。
3. `secret` 字段通过接口读取只返回"已设置"，不返回明文。
4. 升级后参数保留，迁移演练失败时升级被阻断。
5. 设备数据版本高于目标版本时降级被拒，提示可读。
6. 备份恢复后参数与 `persist=1` 的 KV 一致，缓存不恢复。
7. 后装应用订阅已发布点位，安装后立即读到当前值。
8. 发布方未安装时，消费方以 `degraded` 启动，安装不失败；发布方上线后自动恢复。
9. 安装应用 B 前后，应用 A 的包摘要、参数行、KV 行完全不变。
10. 应用尝试读另一个应用的 KV 或参数，返回 `denied_not_granted`。
11. 点位类型或 `contractVersion` 不匹配时订阅被拒，返回 `denied_type_mismatch` 或 `denied_contract_changed`。
12. 超过存储配额或写速率上限时写入失败并告警，不淘汰旧数据，不排队。
13. 发布方改单位并提升 `contractVersion` 后，未重新批准的消费方读取被拒，不静默换算。
14. 卸载发布方前，影响分析列出所有消费方；卸载后它们转 `degraded`，不崩溃。
15. 应用故障后，其点位带 `stale` 或 `bad` 质量，页面不显示为实时值。

## 11. 未决

- 大文件需求出现时再设计 blob 导入；v1 不开放。
- 实测项：`VACUUM INTO` 在目标 SQLite 版本上的可用性；历史降采样在目标板上的 CPU/IO 开销。备份格式、`datasets` 边界、参数变更语义已于 2026-11-09/10 收敛（见 §6/§7）。
