# 03 Host ABI 版本与兼容矩阵

状态：设计基线，待评审。未实现。体积数据为实测，其余为设计目标。

## 1. 版本对象

平台有五个独立版本，不能混成一个"版本号"。

| 版本 | 语义 | 谁声明 | 例子 |
| --- | --- | --- | --- |
| `abi` | Host ABI 的函数集与语义 | WIT 包 | 1.2 |
| `sdk` | 应用面向的开发包版本 | 清单 `sdk` | 1.2 |
| `contribSchema` | 每种贡献类型的 payload 结构 | 注册表 | `telemetry.points/v1` |
| `permModel` | 能力与权限点模型 | 平台 | 1 |
| `runtime_engine` | 运行时实现与其自身版本 | 宿主 | wamr-2.1.0 |

`sdk` 与 `abi` 是独立版本对象，不假定同号。SDK 可以在不改变 ABI 的情况下修 bug 或加辅助 API，ABI 也可以在不发新 SDK 的情况下收紧语义。两者各自按兼容规则校验，绑定的对应关系由 SDK 发布说明登记。

清单把语言、执行档位与运行时引擎分开声明，语言名不得兼作运行时或隔离档位：

```yaml
language: c                 # c | cpp | rust | go | python | js
execution_profile: wasm     # wasm | sidecar | shared（由宿主策略准入，不是自由选择）
runtime_engine: auto        # auto 或宿主支持的引擎名与版本
sdk: "1.2"
minHost: "1.6"
```

宿主公布一个元组 `platform = (host, abi, contribSchema, permModel, engines)`。安装与加载都按元组校验，不接受"语言名等于运行时"的简写。

### 清单字段（2026-11-09 已定）

`app.yaml` 的 JSON Schema v1 以本表为准；05 号文档的交付物不再复述字段。

**必填**

| 字段 | 说明 |
| --- | --- |
| `abi` | `1` |
| `sdk` / `minHost` | 兼容区间 |
| `id` | `[a-z][a-z0-9-]{0,62}`，首次安装绑定 `publisher_id` |
| `version` | semver |
| `publisher_id` | 发布者身份 |
| `language` | `c` / `cpp` / `rust` / `go` / `python` / `js` |
| `execution_profile` | `wasm` / `sidecar` / `shared` |
| `entry` | 包内路径；wasm 固定 `app.wasm`，sidecar 为 `bin/<name>` |
| `limits` | `heapKb` / `storageKb` / `cpuMsPerSec` / `callsPerSec` / `publishPerSec` |
| `capabilities` | 可为空对象；v1 `tags.write` 必须为空 |

**可选**

| 字段 | 说明 |
| --- | --- |
| `runtime_engine` | 默认 `auto` |
| `name` / `description` | 展示用，不参与身份 |
| `products` | 列表，如 `[dcu@1]` |
| `settings` | schema + 默认值 |
| `contributes` | points / ui |
| `requires` | points（产品/应用点位） |
| `fault_policy` | 默认 `auto-restart` |
| `tick_ms` | 缺省不投递 |
| `python` | `{ version: "3.11", requirements: [{name, version, sha256}] }`；`language: python` 必填 |
| `javascript` | `{ bundler: "esbuild@0.21", entry: "app.js" }`；`language: js` 必填 |
| `diagnostics.persistLogs` | 默认 `false` |

规则：

1. 未知字段拒绝，不静默忽略。
2. `id` 不可变；`publisher_id` 首次安装绑定，换发布者必须换 `id`。
3. `entry` 必须在包内且进摘要；包内多余文件默认拒绝。
4. `limits` 必填；宿主只能下调不能上调。
5. 包是单文件容器 `.eapp`：8 字节 magic `EDGEAPP1` + 32 字节 `key_id` + 64 字节 Ed25519 签名 + u64 payload 长度 + payload（tar.zst）。签名覆盖 payload 的 SHA-256；manifest 在 payload 内，不单独签 manifest。
6. `products` 统一为列表；单产品也写列表，避免两种写法。列表中的产品必须共享同一组点位名与语义；语义不同就分别构建，同一个 `app_id`、不同包，各自的 `products` 只有一个产品。

## 2. 兼容规则

ABI 主版本不兼容，次版本向后兼容且只增函数。

| 应用 sdk | 宿主 abi 1.x | 宿主 abi 2.x（弃用窗口内） | 宿主 abi 2.x（窗口期外） |
| --- | --- | --- | --- |
| 1.x | 允许 | 允许，加载时告警 | 拒绝 |
| 2.x | 拒绝 | 允许 | 允许 |
| 3.x | 拒绝 | 拒绝 | 拒绝 |

规则写成可直接编码的判断：

```
兼容 = (app.major == host.major)
      or (app.major == host.major - 1 and host.major 处于弃用窗口)
且 app.minor <= host.minor
```

弃用窗口是在宿主里配置的显式状态，不是"永远支持上一版"。窗口结束后，旧主版本一律拒绝安装与启动。

**窗口长度（2026-11-09 已定）。** 默认 12 个月，且不少于一个固件大版本周期；最短 6 个月、最长 24 个月，由签名固件配置决定，现场与 Edge-Ops 不能延长。ABI 与产品剖面各自计算；窗口外安装与启动都硬拒绝，错误码 `E_ABI_RETIRED` / `E_PROFILE_RETIRED`，不是告警。`minHost` 与窗口独立判定，缺一不可。

## 3. 两层检查

**安装时检查**（静态，失败即拒绝安装）：

- 清单能解析，字段完整。
- `sdk` 落在兼容区间。
- `language` 是已知语言。
- `execution_profile` 是宿主允许的档位，且与该语言的准入策略一致。
- `runtime_engine` 为空、为 `auto`，或指向宿主已安装且版本匹配的引擎。
- `contribSchema` 的每个 `type/schema_version` 宿主都认识。
- 能力在 v1 开放集内，范围不含通配符。
- **清单自洽**：每个非可选 `requires.points` 被某个 `tags.read` 范围覆盖；每个 `contributes.telemetry.points` 被 `points.publish` 覆盖；授权范围里不能出现未声明的点位。不一致返回 `INVALID_MANIFEST` 并指出缺哪条。

**应用间依赖（2026-11-09 已定）。** 允许应用只读依赖：`requires.points` 可列 `app.<provider>.<point>`。安装时按依赖图检查环与深度（≤4），失败返回 `DENIED_DEPENDENCY`；消费者必须显式获得对应 `tags.read` grant，不继承提供方权限。应用之间只读点位，不能调用对方函数或读对方 KV/设置/日志；`points.publish` 仍只写自身命名空间。提供方卸载或未运行时消费者进 `degraded`，读返回 `E_UPSTREAM_UNAVAILABLE`，历史保留，消费者不自动卸载。依赖声明带 `contract_version`；提供方升主版本时消费者要么升级，要么在弃用窗口内保持旧剖面，窗口外拒绝启动。
- 配额字段在允许范围内。

**加载时检查**（动态，失败即拒绝启动，应用标记 `faulted`）：

- 运行时握手返回宿主 `platform` 元组与引擎版本。
- 应用绑定里的 ABI 主版本与宿主一致。
- `execution_profile` 与运行时引擎仍在宿主允许列表内。
- 贡献快照里该应用的贡献 schema 版本仍被支持。
- 授权快照可用，且 grant 的 `(app_id, capability, scope)` 覆盖本次加载；运行时 `app_id`/`publisher_id` 与安装记录一致。
- **WASM 导入白名单**：模块声明的每个 import 必须在白名单内。不在白名单的模块直接拒绝加载，不做"先跑起来再看"。

为什么要两层：安装可能离线完成、可能在旧宿主上进行，U 盘拷到新宿主后静态检查已经不成立。只做安装时检查会漏。

### ABI 总表

所有文档中的能力、设置、事件都从这张表推导。表外的导入一律拒绝。

**宿主导入（模块 `edgeos`）**

| 函数 | 签名要点 | 能力 | 说明 |
| --- | --- | --- | --- |
| `point_resolve` | `(name_ptr, len, out_handle*)` | `tags.read` / `points.publish` | 解析点位名并返回带权限位的句柄：产品点位与他人应用点位查 `tags.read`，自己 `app.<id>.*` 查 `points.publish` 并返回只写句柄。`point_read` / `point_subscribe` 在只写句柄上返回 `DENIED_SCOPE`；句柄带授权代次 |
| `point_read` | `(handle, out_sample*)` | `tags.read` | 返回值、质量、时间戳 |
| `point_subscribe` | `(handle, deadband, min_interval_ms, max_interval_ms)` | `tags.read` | 订阅变化；`max_interval_ms > 0` 时按 keep-alive 至少每 N 毫秒投递一次；回调通过 `app_on_event` 投递 |
| `point_publish` | `(handle, const sample*)` | `points.publish` | 只能写自己命名空间 `app.<id>.*` |
| `setting_get` | `(key_ptr, len, out_ptr, cap)` | 内置 | 读自己的参数，默认值已解析 |
| `setting_watch` | `()` | 内置 | 参数变化通过事件投递 |
| `kv_get` / `kv_set` | `(key, value, len, flags)` | 内置，受配额 | 自己的命名空间 |
| `log` | `(level, ptr, len)` | 内置 | 每应用 100 行/秒、单行 4KB；64KB 内存环；可选持久化 1MB×2 轮转 |
| `now` | `(out_mono_ms*, out_wall_ms*)` | 内置 | 两种时钟分开 |
| — | — | — | **没有运行时 `timer_set`**；周期执行见下方"订阅与定时" |

**应用导出**

| 函数 | 时机 | 说明 |
| --- | --- | --- |
| `app_init` | 实例加载后、接收事件前 | 读设置、解析点位句柄 |
| `app_on_event` | 点位变化、平台事件、定时（仅当清单声明 `tick_ms`）、用户动作 | 单线程串行调用，不重入 |
| `app_stop` | 停止前 | 落盘，限时返回 |
| `app_migrate_settings` | 升级激活前（可选） | 旧 settings → 新 settings，纯函数，限时限资源 |
| `app_migrate_data` | 升级激活前（可选） | `persist=1` 的 KV → 新格式，纯函数 |

事件载荷统一为 `{type, id, mono_ms, wall_ms, payload}`。类型含点位变化、设置变化、授权变化、上游可用或不可用、代次变化、即将停止、定时 tick、用户动作 action。点位事件额外带 `reason`：`change`（越过死区）、`keepalive`（到 `max_interval_ms` 但值没变）、`quality`（质量变化，永远投递，不受死区限制）。

### 订阅与定时

- `point_subscribe` 的 `deadband` 是绝对值；`min_interval_ms` 限流；`max_interval_ms > 0` 时即使值不变也按周期投递 keep-alive，用于“按时间结算”的应用。三者同时生效时，先过限流，再看死区，最后看 keep-alive 到期。
- 订阅不自带回放。应用要当前值先 `point_read` 再订阅；两者之间的变化由 keep-alive 或下一次变化补齐。这也符合 08 号文档"事件不是真相、状态可重读"。
- **点位事件按点合并**：队列里每个点位最多一条，新样本覆盖旧样本；平台事件不合并。应用总是看到最新值，质量变化通过样本自带的质量字段判断，不单独发粘性事件。
- **没有运行时 `timer_set`。** 周期执行有两条路：按点位的 `max_interval_ms`，或清单可选字段 `tick_ms`（缺省不投递）。`tick_ms` 只用于“周期与任何点位无关”的应用；宿主校验下限/上限（初值 50ms–60s）并计入 Host 调用预算。
- tick 不重入：上一次 `app_on_event` 未返回时，下一次 tick 不投递，记一次"错过"并计入指标。

### 发布与质量

- `point_publish` 的 sample 含 `value`、`quality`（good/bad/uncertain）与可选 `source_ts`（毫秒，0 表示不提供）。
- **句柄带权限位**：`read` 句柄（产品与他人点位）只能 read/subscribe；`publish` 句柄（自己的 `app.<id>.*`）只能 publish。用错返回 `DENIED_SCOPE`。这是 `open(2)` 的 `O_RDONLY`/`O_WRONLY` 模型，不是所有句柄都能做所有事。
- 宿主在接收时盖 `server_ts`（单调钟），并校验 `source_ts` 落在 `[now-1h, now+1s]`；越界返回 `INVALID`。
- 应用不能设置 `stale`。`stale` 由读侧按 `server_ts` 与点位的 `stale_after_ms` 计算；应用故障或停止后无需推送。
- **`PipeMsg` 只有一个 `ts` 字段，64 字节契约不能改**：`PipeMsg.ts` 存 `server_ts`；`source_ts` 存在产品进程的 app-point 运行目录（`slot → source_ts`、`stale_after_ms`、owner），`point_read` 返回两者；只经 DataHub 影子表消费的进程（如 Go）只看到 `server_ts` 与质量。这是有意的降级，写进 06 号文档。
- 类型与单位在 `point_resolve` 时按清单与产品剖面校验；发布值类型不匹配返回 `INVALID`。
- 每点每秒发布次数有上限；超限返回 `DENIED_QUOTA`，不排队。应用不能发布到产品点位或其他应用命名空间（`DENIED_SCOPE`）。

### 动作（actions）

- 清单 `ui.actions` 声明 `{ id, label, confirm, permission }`；`id` 语法同点位名的段规则。
- 触发时宿主校验三件事：用户具备 `permission`（默认 `app.<id>:configure`）、应用实例在运行、清单里声明了该 action。任一失败返回 `denied` 并写审计。
- 通过后投递 `app_on_event(type=action, payload={action_id, args, request_id})`。action 事件不合并，按顺序投递。
- **v1 是"触发即完成"**：`app_on_event` 没有返回值；执行结果由应用写日志、发点位或写审计体现。UI 立即显示"已下发"，不等待结果。需要结果时，应用把结果发布成点位，页面订阅。
- 不做新的 `app_on_action` 导出，避免两套回调。

### 升级迁移

- `app_migrate_settings` 与 `app_migrate_data` 是**可选导出**。没有导出时只允许“加法变更”：新增字段带默认值；检测到删除字段、改类型或改语义，升级被拒。
- 迁移在激活前跑：宿主起一个沙箱实例，把现有 `app_settings` 与 `persist=1` 的 KV 传进去；实例标 `migrating`，不对外可见。超时或失败则升级阻断，旧版本继续服务。
- 迁移是幂等的，允许重跑；结果写 `app_data_meta` 的 `settings_version` / `data_version`。
- CI 的“迁移演练”直接调用这两个导出，不用起完整平台。

**错误码**

```
DENIED_NOT_GRANTED  DENIED_SCOPE  DENIED_REVOKED  DENIED_POLICY
DENIED_QUOTA  NOT_FOUND  STALE  BUSY  INVALID  INTERNAL
```

**规则**

- 所有指针与长度由宿主做边界检查；单次载荷有上限（初值 64KB）。
- 同一应用同一时刻只有一个调用在执行；订阅回调不重入。
- **每个 Host 调用有 deadline**：`point_read` / `point_publish` 100ms，`kv_set` 500ms，`app_init` 1s，`app_stop` 5s。超时返回 `BUSY`；读一个没有值的点位返回 `STALE`。迟到的回包按 `seq` 丢弃，不投递给已经超时的实例。单次超时不直接判故障，连续超时计入故障连击。sidecar 的 RPC 客户端用同一张 deadline 表。
- 返回码不跨边界抛原生异常；trap 统一映射为 `INTERNAL` 并计一次故障。
- 内存归宿主所有，应用只读；需要保留的值由应用自己复制。
- 授权撤销后，已有句柄在下一次调用返回 `DENIED_REVOKED`，不依赖应用重新解析。

**能力与函数的对应**

| 能力 | 允许的函数 | 范围 |
| --- | --- | --- |
| `tags.read` | resolve / read / subscribe | 批准的点位名 |
| `points.publish` | publish | 仅 `app.<id>.*` |
| `settings.read` / `settings.write` | setting_get / watch；写入只经 API | 自己的键 |
| `kv` | kv_get / kv_set | 自己的命名空间 |

`points.publish` 与 `tags.write` 是两件事。前者只能写自己命名空间的派生点位，后者写产品点位，属于遥控类高风险，P0 不开放。混成一个能力会让"发自己的计算结果"和"改现场设备值"共用一条审批。

**WASI 白名单**

只允许 `clock_time_get` 与 `random_get`。拒绝文件、socket、环境变量、进程相关导入；拒绝 `wasi:threads`、共享内存、`memory64` 与任何自定义 import。拒绝发生在实例化之前，错误码 `DENIED_IMPORT`。因此应用没有文件系统；v1 也不提供宿主 blob API，大文件不开放（见 06 号文档）。

这项检查防的是"沙箱里再开一扇门"。只靠线性内存隔离、不在加载时拒绝越界 import，等于把沙箱交给模块自觉。

### 与现有 `env` ABI 的关系

仓库现有 `docs/wasm_micro_app_guide.md` 与 `examples/wasm/*` 使用的是一套更早的 ABI：模块 `"env"`，函数 `ext_datahub_read(tag_id, ...)`、`ext_datahub_publish(tag_id, ...)`，点位是**全局数字 TagID**（如 1001、9999）。

这套 ABI 与 ADR-0010 冲突，不能作为新平台的 ABI 继续扩展。**已定（2026-11-09）：拒绝旧模块，不提供兼容层。** 未声明 `abi: 1` 或声明 `abi: 0` 的模块安装即拒，错误码 `DENIED_ABI`。

理由：兼容层会把 ADR-0010 刚废除的全局数字标识重新引回业务路径；仓库里没有真实第三方存量（`edge_app_mgr` 未被任何产品构造，实际在跑的 `stage_wasm` 与 smart_box 的硬编码应用都不是可迁移的第三方包）；iOS 停 32 位、VS Code 的 `engines.vscode` 区间都是硬性最低版本，不是永久兼容层。`examples/wasm/*` 与两篇指南标 `legacy`，并给一个最小迁移示例。若现场真的盘出旧应用，再单独评审兼容层，不在 v1 默认。

### 包体限制与校验顺序

签名验证必须在解析清单之前，解析器必须有界。否则一个构造过的包可以在验签前耗尽内存或栈。

- 容器头先给 `key_id`，按它从设备信任库取公钥；`key_id` 不在信任库直接拒绝，不尝试遍历所有公钥。
- 先验签，再解包，再解析。顺序不可交换；签名覆盖 payload 的 SHA-256。
- 解压后总大小、文件数、单文件大小、路径深度都有上限；超出即拒，不解压到磁盘。
- 拒绝绝对路径、`..`、符号链接、硬链接、重复路径、设备文件，防止写到包目录之外。
- 压缩比 ≤100:1，防 zip bomb；文件数 ≤256、单文件 ≤8MB、解压后总量 ≤64MB。
- JSON 解析限制深度、键数、字符串长度；不接受注释与重复键。
- manifest 的 `files[]` 列出 payload 内全部文件及 SHA-256，用于安装期二次校验与审计；它不是信任根，信任根是 payload 签名。包内出现 `files[]` 之外的文件即拒绝。

这些是供应链侧的常规防御，和运行时隔离是两件事。只做运行时隔离，不管包体，等于让恶意包在安装阶段就把宿主拖垮。

### 供应链与防回滚

- 签名覆盖整个 payload 的 SHA-256；manifest 与文件摘要都在 payload 内，随 payload 一起被认证。
- 版本只增不减：宿主记录每个应用见过的最高版本，安装更低版本即拒绝，除非管理员显式执行"降级"并写审计。这是防回滚攻击的最小实现。
- 公钥轮换：支持同时信任新旧两把公钥，轮换期内两者都可验签，轮换完成后移除旧钥。轮换过程需要一次带外确认，不能由包自己声明。
- 可选 SBOM：包内附带依赖清单，安装时记录，不做强制校验。现场离线环境里，SBOM 的价值在追溯，不在准入。

**应用标识绑定发布者。** 引入 `publisher_id`，它是一把长期身份密钥；`app_id` 首次安装时绑定 `publisher_id`，写入注册表并进审计。每次发布用 `publisher_id` 的当前签名密钥，密钥链由旧钥签新钥，轮换不需要换 `publisher_id`，也不需要换 `app_id`。吊销按签名密钥指纹生效，不按 `publisher_id`。

只有 `publisher_id` 本身被转移时才需要换 `app_id`，因为新发布者不能继承旧 id 的历史点位映射与已有授权。没有这层区分，发布者自己轮换签名密钥会被当成"另一个发布者"，把正常运维变成迁移事件。

**信任根与吊销。** 设备内置一组受信公钥，随固件更新；额外公钥只能由管理员通过带外方式导入，不能由包声明。吊销列表是一个签名文件，支持离线导入，记录被吊销的密钥指纹与时间；导入后立即生效并写审计。`min_security_version` 是单调下限，低于它的版本即使签名有效也拒绝安装。

**信任根的落地方式（2026-11-09 已定）。**

1. **厂商根公钥烧进固件**，私钥离线保存（HSM 或离线机），不进仓库。固件升级可以带新根，但必须由旧根签名。
2. **第一方应用**由厂商发布密钥签名，证书链到根；`publisher_id` 首次安装时绑定。
3. **第三方发布者公钥**由管理员带外导入：`appctl trust import <key>`（可要求厂商根对该 key 签名），导入写审计；或由 Edge-Ops 下发。包不能自己声明信任，包内自带根一律拒绝。
4. **吊销列表**由厂商根签名，离线导入，立即生效。
5. **仓库里的 `keys/edgec_signing_private.pem` 是测试密钥**，必须标 `test-only`；CI 检查发布包不能用它签名，正式发布密钥不出现在仓库。

**运行时选型与 WIT 的关系。** WIT 是接口描述，不等于必须使用 Component Model。默认运行时是 WAMR，因此把 WIT 当 IDL，生成 core-wasm 绑定；只有 wasmtime 作为备选后端时才使用 Component Model 与 `componentize-py`。这个选择在选型 ADR 里按当前版本核实，不能在不同文档里各说一套。Go 的 RPC 客户端需要自建 WIT 到 RPC 的生成器，成本要计入实施预算。

## 4. 生成绑定与 CI

绑定由 WIT 生成，不允许手改。

- 每个语言一个生成目录，文件头带 `// Code generated by wit-bindgen from abi/1.2. DO NOT EDIT.`
- CI 步骤：重新生成到临时目录，与仓库中的绑定做 `diff`，有差异即失败。
- 生成器的版本与 WIT 包版本一起锁定在 `tools/abi/pins.json`，升级生成器需要单独提交。
- **仓库布局（2026-11-09 已定）**：`abi/<major>.<minor>/edgeos.wit` 是唯一契约；`sdk/c/generated/`、`sdk/go/edgeosabi/`、`sdk/python/edgeos_abi/` 生成入库；v1 不发布语言包，需要时从同一 tag 打包，版本随 ABI 主版本，不独立演进；WIT 次版本只加不改，主版本另开目录并保留弃用窗口。
- 手改检测：除重新生成比对之外，再加一条"生成文件不得出现在人工改动的 PR 里"的检查，用文件头标记判断。

各语言路径，档位由 `execution_profile` 决定，不由语言名决定：

| 语言 | 工具 | 产物 | 典型档位 |
| --- | --- | --- | --- |
| C | clang wasm32 + wit-bindgen C | WASM | `wasm` |
| C++ | 同上，`-fno-exceptions -fno-rtti`；`sdk/cpp/` header-only RAII 包装 | WASM | `wasm` |
| Zig | `@cImport` 消费 C 头 | WASM | `wasm` |
| Rust | wit-bindgen Rust，`panic=abort` | WASM | `wasm` |
| Go | 按同一 WIT 生成 RPC 客户端；`CGO_ENABLED=0` 静态二进制 | 原生二进制 | `sidecar` |
| Python（P1） | CPython venv（`--copies --without-pip`）+ 生成的 RPC 客户端 | 原生进程 | `sidecar` |
| JavaScript（P2，QuickJS） | `edgeos.js` 包装模块 | JS 脚本 | `sidecar`（第三方默认）；`shared` 仅第一方受信 |

Go 不默认编 WASM。实测一个最小 Go 应用编 wasip1 剥符号后 1,642,824 字节，gzip 492,130 字节；同一逻辑的 C 产物 281 字节。差约 5,800 倍，因此标准 Go 走原生 sidecar，ABI 通过 RPC 暴露，函数签名与 WIT 一致。

TinyGo 是另一个编译器与运行时，产物通常几十 KB，可以走 A 档。它与标准 Go 不是同一件事，不能因为都叫 Go 就放在同一档。选 TinyGo 的代价是标准库子集与 goroutine 行为受限，需要单独验证。

体积数据只说明 hello 级差异，不足以单独决定档位。真正的判据是 RSS、启动时间和 RPC 往返，这三项必须在 P-1 spike 里测，不能拿体积代替。

### SDK 交付边界（2026-11-09 已定）

- **一份契约，两层交付**：WIT 是唯一契约；每语言交付 = 生成的类型/函数绑定 + 一个薄运行时适配器（约 300–800 行），不写框架、不写业务抽象。
- **三种适配器，不是五种**：WASM core（C/C++/Rust/Zig）生成绑定直接调 host imports；native sidecar（Go/Python）共用一套 socketpair RPC 协议，每语言只实现事件循环、心跳、句柄与 `app_init`/`app_on_event`/`app_stop` 注册；进程内 JS（QuickJS）用一个 `edgeos.js` 包装模块暴露 imports 与回调，不走 socket。
- **SDK 内容固定**：类型、生命周期签名、句柄助手、settings/KV/log 包装、测试 harness；不加 ORM、Web 框架或调度器。
- **API 形状（2026-11-09 已定）**：每语言提供 `App`（`init(ctx)` / `on_event(event)` / `stop()`）、`Point<T>`（`resolve` / `read` / `subscribe` / `publish`，析构释放）、`Context`（`settings` / `kv` / `log` / `now()`）三层薄封装；不暴露原始 fd、令牌或 DataHub；错误用类型化枚举并保留 `DENIED_*` 原始码；`subscribe` 不另起线程。不做依赖注入、ORM、自动重试或隐藏线程池。
- **C++ / Zig（2026-11-09 已定）**：不单独发布 SDK。C++ 用 `sdk/cpp/` 的 header-only RAII 包装消费 C 绑定（`-fno-exceptions -fno-rtti`）；Zig 用 `@cImport` 直接消费 C 头。二者是 C SDK 的消费者，构建矩阵只保证 C/C++ 的 WASM 产物，不承诺异常、RTTI 或完整标准库。
- **开发宿主 `edgeos-sim`（2026-11-09 已定）**：x86_64 Linux 单进程，加载同一 app-host + WAMR、假 `dcu@1` 剖面与内存 DataHub；`edgeos-sim run ./app.eapp` 拉起并打印事件与日志。默认给清单声明的全部能力，但明确打印"sim 不校验 grant"，不能当安全证明；支持 `--fault timeout|crash|denied`，与 test-platform 共用场景定义。不进生产镜像，P1 交付，CI 用它跑 SDK conformance。
- **打包工具 `edgeos-pack`（2026-11-09 已定）**：开发机/CI 工具，输入 `app.yaml`、产物与私钥，输出 `.eapp`；负责 canonical tar（排序、`mtime=0`、uid/gid=0）、zstd、payload 哈希、Ed25519 签名与容器头。打包前校验 manifest schema、清单自洽、文件数/大小/压缩比、symlink 拒绝、Python venv 的 `pyvenv.cfg`/`dist-info`、JS 单文件；失败不入包。默认可复现，私钥不落日志；设备只验签解包，不打包。
- **单元测试 `MockContext`（2026-11-09 已定）**：每语言提供内存版 `Context` / `Point<T>`，`publish` 记录到数组、`read` 返回预设值、`subscribe` 可手动投递；支持 `emit(event)` 与 `advanceClock(ms)`，测试不 sleep。mock 与真 SDK 共用 WIT 生成类型，只替换传输层；不模拟授权、配额或进程隔离，CI conformance 仍用真宿主。
- **顺序**：C（P0）→ Rust、Go、Python（P1；Python 是必须支持项，不因板型拒绝）→ JS（P2）。未承诺的语言不进构建矩阵。
- **CI**：同一个 conformance 应用跑每种语言；生成代码入库并 diff；适配器补丁版本独立于 ABI 次版本，主版本跟 ABI；清单 `sdk` 字段声明应用使用的版本。

**Python 打包（2026-11-09 已定）。** 每个 Python app 一个 venv：构建期 `python -m venv --copies --without-pip venv/`，依赖装进 `venv/lib/python3.11/site-packages/`；`venv/` 随 `.eapp` 签名，安装解到 `/var/edgeos/apps/<id>/venv-<version>/`；运行 `venv/bin/python -E -s -B -m app_main`，`PYTHONNOUSERSITE=1`。共享系统 stdlib，不共享 site-packages；清单 `python.version` 不匹配拒绝安装。构建容器的 CPython 版本/ABI 必须与目标镜像一致，`pyvenv.cfg` 的 `home` 重写为目标 `/usr/bin`；打包时解引用 venv 内符号链接，最终 tar 不含 symlink，否则安装拒绝。设备不跑 pip、不联网；`python.requirements` 记录名称/版本/哈希，安装校验 `dist-info`。Python 包上限 64MB（解压 128MB），每应用磁盘配额 256MB（含 venv），超限拒绝安装；升级建新 venv，旧 venv 保留到回滚期。

**JS 打包（2026-11-09 已定）。** 构建期用 esbuild/rollup 把应用与纯 JS 依赖打成单文件 `app.js`，`node_modules` 不进包；只允许纯 JS 依赖，原生插件（`.node`）与 `fs`/`net`/`child_process` 等 Node 内置模块一律拒绝。运行时没有 `require` 动态解析、没有 `node_modules` 查找，只提供 `edgeos` 模块与标准 ES 子集；应用不能 `eval` 加载远程代码。JS 包上限 16MB，超出拒绝。

## 5. 贡献 schema 迁移

贡献 schema 升级不能悄悄改语义。每种类型必须显式声明兼容模式，不能只写一个版本号。

| 模式 | 含义 | 允许的变更 | CI 检查 |
| --- | --- | --- | --- |
| `backward` | 新读者能读旧数据 | 只加可选字段 | 旧样本喂新 schema 必须通过 |
| `forward` | 旧读者能忽略新数据 | 只加字段且旧读者忽略未知字段 | 新样本喂旧 schema 必须通过 |
| `full` | 双向兼容 | 只加可选字段 | 两个方向都测 |
| `none` | 不承诺 | 任意 | 升级必须走迁移演练 |

v1 的 `telemetry.points` 采用 `backward`，因为消费方会随宿主一起升级。声明 `none` 的类型必须提供迁移函数。

- 每种类型有 `vN`，注册表存 `schema_version`。
- 宿主为每个 `vN → vN+1` 提供纯函数 `migrate(payload)`，可单测。
- 升级宿主前，跑一次"迁移演练"：把所有现存的 `vN` payload 在内存里迁移一遍，任何一条失败就不允许升级。
- 迁移是幂等的，允许重跑。
- 迁移不删除旧字段，先写新字段，下一主版本再删。
- 未知字段在写路径上拒绝，不静默丢弃；静默丢弃会让升级看起来成功，实际丢数据。

这套规则对应 protobuf 与 Avro 的兼容性模型。选择显式模式而不是"尽力而为"，是因为数据一旦写进历史存储，回滚 schema 的代价远高于升级前多跑一次检查。

## 6. 回滚能力

回滚要求旧版本仍然可运行。

- 保留上一个 `app_version` 的包与 manifest。
- 保留上一个 ABI 主版本的运行时，至少一个弃用窗口。
- 回滚只切 `desired_version` 与 `published_version`，推进一个新 generation，不重新构建。
- 若旧版本依赖的 ABI 已不在宿主里，回滚被拒，并明确提示"不可回滚，需降级宿主"。

## 7. 兼容矩阵怎么测

全组合测不起，所以抽样并标注。

矩阵维度：`abi` 主版本（当前与上一）、`sdk` 次版本（当前与上一个）、`contribSchema`（当前与上一个）、运行时（WASM 宿主必测，sidecar 各自接入时测）。v1 的 ABI conformance 断言放 EdgeC `tests/`；端到端兼容矩阵跑在 test-platform 的 `edgec-app` 任务里。

发布时跑：

1. 当前组合全测。
2. 上一主版本 ABI × 当前 sdk，各语言的示例应用各一个。
3. 当前 ABI × 上一贡献 schema，迁移演练。
4. 未覆盖组合在发布说明里标 `unverified`，不得写"支持"。

标签要出现在三个地方，不只写在发布说明：安装界面对未覆盖组合显示"未验证"；`appctl status` 输出 `compat: verified | unverified`；升级提示里同样标注。只在文档里标注，现场看不到，等于没标。

## 8. 验收

1. `sdk 2.x` 的应用在 `abi 1.x` 宿主上被拒，错误码明确。
2. 弃用窗口内 `sdk 1.x` 应用可运行并产生告警日志。
3. 窗口外同一应用被拒。
4. 手改一个生成绑定文件，CI 失败。
5. 清单声明未知 `contribSchema`，安装被拒。
6. 旧 schema 数据在升级演练中全部迁移成功，任何一条失败时升级被阻断。
7. 回滚到上一版本成功，且运行时仍存活。
8. 回滚到已移除 ABI 的版本被拒，提示可读。
9. 同一份 C 应用在宿主 ABI 1.2 与 1.3 上行为一致。
10. Go sidecar 与 C WASM 应用通过同一套 conformance 用例。
11. 清单声明 `language: go` 但 `execution_profile: wasm`，且宿主策略不允许该组合时，安装被拒。
12. `sdk 1.2` 配 `abi 1.3` 且 SDK 发布说明未登记该对应关系时，安装被拒并给出可读原因。
13. 用不同发布者密钥签同名 `app_id` 的包，安装被拒。
14. 导入吊销列表后，被吊销密钥签名的包立即拒绝安装与启动。
15. 低于 `min_security_version` 的包即使签名有效也被拒。
16. 调用 ABI 总表之外的导入，模块在实例化前被拒。
17. `point_publish` 写非本应用命名空间的点位，返回 `DENIED_SCOPE`。
18. 撤销授权后，已持有的句柄下一次调用返回 `DENIED_REVOKED`。
19. `max_interval_ms > 0` 时，点位值不变也在周期内收到 `keepalive` 事件。
20. 质量从 good 变 bad 时，即使变化量在死区内也投递 `quality` 事件。
21. 清单未声明 `tick_ms` 时不投递 tick；声明的值超出下限/上限时安装被拒。
22. 上一次 `app_on_event` 未返回时，下一次 tick 被跳过并计数，不重入。
23. 产品进程人为卡住 500ms，`point_read` 在 100ms 返回 `BUSY`，实例不被挂死。
24. 迟到的回包不投递给已超时的调用；`seq` 不匹配的帧被丢弃。
25. `app_init` 超过 1s，实例标 `faulted`，不进入 active。
26. `source_ts` 早于 1 小时前或晚于 1 秒后，发布返回 `INVALID`。
27. 应用发布 `quality=bad` 后，读者拿到 bad；应用故障后，读者按 `stale_after_ms` 拿到 stale，不需要故障进程推送。
28. 只读 DataHub 影子表的 Go 消费者看到 `server_ts` 与质量；`point_read` 额外返回 `source_ts`。
29. 用仓库内测试私钥签名的发布包被 CI 拒绝；包内自带信任根被安装拒绝。
30. 第三方公钥由无 `app.trust.import` 权限的账号导入，被拒并写审计。
31. 解析自己的点位返回只写句柄；在它上面 `point_read`/`point_subscribe` 返回 `DENIED_SCOPE`，`point_publish` 成功。
32. 清单没有 `points.publish` 范围时解析自己的点位，返回 `DENIED_NOT_GRANTED`。
33. 在只读句柄上 `point_publish`，返回 `DENIED_SCOPE`。
34. 升级删除字段且未提供 `app_migrate_settings`，升级被拒，旧版本继续服务。
35. 迁移导出超时，升级阻断；重跑迁移结果一致。
36. 迁移后 `settings_version` / `data_version` 前进，旧数据按新格式可读。
37. 应用 A 依赖应用 B 的点位：B 未启动时 A 为 `degraded`，B 启动后恢复；环依赖与深度超 4 的安装被拒。
38. 应用 A 读应用 B 的点位，未获 `tags.read` 授权时返回 `DENIED_NOT_GRANTED`，不能隐式继承 B 的权限。

## 9. 未决

- Go 的 RPC 客户端与 WASM 绑定是否共用同一份 conformance 断言，目前只是目标，未实现。
- 贡献 schema 是否需要跨类型的联合迁移，例如点位与北向映射同时升级。
