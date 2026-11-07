# 后装应用平台设计：贡献点 + 沙箱运行时

状态：提案，待评审
范围：后装应用（WASM / JS / 未来 Python）如何与 WebUI、北向、告警、存储、调度等子系统协作
不改变：现有 DataHub、北向端口、构建期 Registry 的既有权衡

## 1. 问题

现在每接一类新东西，都要在多个子系统各改一处。装一个电压越限应用，WebUI 要加页面，北向要加点位映射，告警要加规则，存储要加表。应用数 M 乘子系统数 N，工作量是 N×M，而且每加一次都要重新发版、重新验收。

目标是把 N×M 降到 N+M。应用只写自己的一份清单和一份逻辑；子系统只实现一次"按贡献类型消费"，之后不认识任何具体应用。

## 2. 结论：VS Code 的清单，加 iPhone 的沙箱

两边的做法各取一半，并且都明确不取另一半。

取 VS Code 的部分，是**声明式贡献点**。扩展在 `package.json` 里声明 `contributes.commands`、`contributes.views`、`contributes.configuration`；核心不认识任何扩展的名字，只按贡献类型把它们挂上去。这是把 N×M 变成 N+M 的关键机制。

取 iPhone 的部分，是**包、签名、能力声明、沙箱、生命周期**。应用是签名的安装包，声明自己需要的能力，在受限环境里跑，有安装、启动、停止、卸载、升级、回滚。

不取 VS Code 的部分，是**让扩展在特权进程里跑任意 JS**。那等于给应用全部宿主机权限，现场设备不能接受。

不取 iPhone 的部分，是**每个应用自带一套 UI**。WebUI 必须是一个连贯的操作面，应用只能贡献声明式的数据与页面描述，由通用渲染器画出来。这一条是刻意的偏离，也是最容易做错的地方。

一句话：**应用声明贡献，核心按类型消费；应用在沙箱里跑，不碰宿主。**

## 3. 现有可复用的四个支点

这套方案不是另起炉灶，仓库里已经有四个支点。

- `modules/edge_mgmt/include/edgeos/edge_app_mgr.h` 已有应用清单（id、name、version、author、触发点位、入口函数、堆上限、超时、故障策略）和安装、启动、停止、卸载、开机恢复、状态机。
- `modules/edge_io/include/edge_north_plane.h` 的北向端口已经有 `peek`、`name`、`resolve_name`、`accept`、`on_downlink`，北向编排只按端口取数，不认识产品点位模型。
- `go/cmd/edge-gateway/main.go` 已经有服务注册：服务上报 `Tags` 和 `UIConfigs`，网关融合后由 `/api/v1/ui/bootstrap` 一次性下发。这就是贡献注册表的雏形。
- `web/core/src/modules` 下的页面、`modules.manifest.yaml` 的构建期注册表、`permissions/catalog.yaml` 的权限目录，已经定义了页面、权限、能力怎么被表达。

要做的不是新造一套，而是把 `ui_configs` 这种"谁都能塞一段 JSON"的弱约定，升级成带类型、带版本、带校验的贡献注册表。

## 4. 目标架构

分四层，层间只有一条不变式：

> 子系统只认识贡献类型，不认识应用名。

```
┌────────────────────────────────────────────────────┐
│ L3 子系统适配器（各实现一次，之后不再改）           │
│  WebUI 页面渲染 / 北向映射 / 告警 / 存储 / 调度      │
└───────────────▲────────────────────────────────────┘
                │ 只读：按 contribution type 查询
┌───────────────┴────────────────────────────────────┐
│ L2 贡献注册表（唯一事实来源）                       │
│  类型 + 版本 + 所有权 + 校验 + 生效/失效            │
└───────────────▲────────────────────────────────────┘
                │ 安装/激活时写入，卸载时撤销
┌───────────────┴────────────────────────────────────┐
│ L1 应用宿主与运行时适配                             │
│  WASM / QuickJS / MicroPython，同一套 Host ABI      │
└───────────────▲────────────────────────────────────┘
                │ 宿主函数：tag / alarm / kv / log
┌───────────────┴────────────────────────────────────┐
│ L0 内核：DataHub、事件总线、SQLite、审计、权限       │
└────────────────────────────────────────────────────┘
```

不变式的含义很具体：WebUI 不需要知道"电压越限守护"这个应用存在，它只需要会渲染 `ui.page` 类型；北向不需要知道应用名，它只需要遍历 `northbound.metric` 类型。新增一个应用，是往注册表加行，不是给五个子系统加分支。

## 5. 应用包与清单

应用包是一个目录或 zip，根目录放 `app.yaml` 和代码。

```yaml
schemaVersion: 1
id: voltage_guard
name: 电压越限守护
version: 1.4.2
author: EdgeOS 官方
runtime: wasm            # wasm | js | python
entry: app.wasm          # 或 app.js / main.py
sdk: 1.0                 # 应用面向的 Host SDK 版本
minHost: 1.6             # 最低宿主版本

# 能力即权限：没有声明就没有宿主函数
capabilities:
  tags:
    read:  [U_A, U_B, U_C]
    write: []
  alarms:
    raise: [power.voltage]
  kv:
    namespace: app.voltage_guard
  schedule:
    jobs: []

limits:
  heapKb: 512
  timeoutMs: 200
  cpuMsPerSec: 20
  storageKb: 256
  faultPolicy: auto-restart   # auto-restart | isolate-and-alarm

# 贡献点：核心按类型消费
contributes:
  telemetry.points:
    - id: app.voltage_guard.sag_count
      name: 电压暂降次数
      unit: 次
      type: uint32

  ui.pages:
    - id: voltage-guard
      title: 电压越限
      group: apps
      permission: app.voltage_guard:read
      layout:
        - kind: metric-row
          items:
            - { label: 今日越限, value: app.voltage_guard.over_count, unit: 次 }
            - { label: 最长暂降, value: app.voltage_guard.max_sag_ms, unit: ms }
        - kind: table
          source: app.voltage_guard.events
          columns:
            - { key: ts, title: 时间 }
            - { key: phase, title: 相别 }
            - { key: depth, title: 跌落深度, unit: '%' }

  northbound.metrics:
    - source: app.voltage_guard.sag_count
      metric: voltage_guard_sag_count
      type: double
      rw: r

  alarm.rules:
    - id: voltage.sag
      source: app.voltage_guard.last_sag
      condition: value > 0.1
      severity: warning
      message: 检测到电压暂降
```

清单是唯一入口。代码里没有"注册页面"这种调用，注册发生在安装时，由宿主按清单写入注册表。这样卸载只需按 `id` 撤销全部贡献，不会有残留。

## 6. 贡献点目录

先只开六个类型，覆盖当前所有需求，未列入的等真实场景再开。

| 类型 | 写给谁 | 子系统读法 | 变更影响 |
| --- | --- | --- | --- |
| `telemetry.points` | DataHub 元数据 | 点位表合并静态与应用点位 | 北向、历史、告警自动可见 |
| `ui.pages` | WebUI 动态路由 | 通用渲染器按 layout 画 | 不加页面代码 |
| `northbound.metrics` | 北向映射表 | 北向按 metric 遍历 | 不改北向编排 |
| `alarm.rules` | 告警引擎 | 规则表合并 | 不改告警服务 |
| `schedule.jobs` | 任务调度 | 任务表合并 | 不改调度器 |
| `protocol.handlers` | 南向协议栈 | 探测链追加处理器 | 仅驱动层读 |

六个类型都是"数据"，没有一个是"代码注入"。这是可控的关键：子系统消费的是描述，不是可执行片段。

`ui.pages` 的 layout 只允许封闭的组件集合：指标行、表格、状态列表、时序图、表单、分段控件。每个组件在 WebUI 里已经存在，渲染器只是把 JSON 映射到这些组件。应用不能写模板、不能写 CSS、不能跑前端脚本。

## 7. 运行时适配层

`runtime: wasm | go | js | python` 四个值，背后是一个 C 接口，应用侧看到的 API 完全一致。宿主怎么跑它（进程内还是 sidecar）由运行时决定，见第 16 节。

```c
typedef struct {
    const char *name;                 /* "wasm" / "js" / "python" */
    int  (*load)(void *inst, const app_package_t *pkg);
    int  (*start)(void *inst);
    int  (*invoke)(void *inst, const char *func, const uint8_t *in, size_t in_len,
                   uint8_t *out, size_t *out_len);
    int  (*stop)(void *inst);
    void (*unload)(void *inst);
} app_runtime_ops_t;
```

- WASM 用 wasmtime 或 WAMR，线性内存天然隔离，堆上限直接映射为 memory limit。
- JS 用 QuickJS，禁掉 `eval` 以外的宿主对象，只注入 Host ABI。
- Go 走 sidecar 进程，用原生 Go 构建，不编 WASM。实测体积差见第 17 节，随包携带 GC 运行时不划算。
- Python 走 sidecar 进程，用 MicroPython 或 CPython 视内存预算而定。两者都通过 RPC 暴露同一套 Host ABI。

三个运行时都要过同一套一致性测试：同一份 `app.yaml`、同一组 Host ABI 调用、同一组配额与故障用例，结果必须一致。不做"WASM 能做、Python 不能"的分叉，否则应用会被运行时绑死。

## 8. Host ABI

应用不链接 DataHub，不打开 SQLite，不建 socket。它只调用宿主注入的函数。

```
tag.read(tag_id) -> value
tag.write(tag_id, value) -> ok | denied        # 需 capabilities.tags.write
tag.subscribe(tag_id) -> ok                     # 事件驱动入口
alarm.raise(rule_id, payload) -> ok
kv.get(key) / kv.put(key, value)                # 命名空间隔离
log(level, message)
now_ms() -> u64
```

每个函数在调用时校验当前应用的能力声明。`tag.read` 没在 `capabilities.tags.read` 里的点位返回 `denied`，并且写一条审计。这就是"能力即权限"，不是靠应用自觉。

ABI 版本化。`sdk: 1.0` 的应用只能看到 1.x 的函数集；加函数是次版本，改语义是主版本。宿主在安装时就拒绝 `sdk` 不兼容的包，而不是运行到一半崩溃。

## 9. WebUI 怎么不改

WebUI 现在有两套路由来源：构建期 Registry（`modules.manifest.yaml`）和产品页面。应用页面是运行期的，所以要加第三条来源，但只加一次。

1. 登录后，Shell 调 `/api/v1/apps/contributions?type=ui.pages`。
2. 返回的是经过校验的页面描述，不是代码。
3. Shell 把它们挂到一个固定的"应用"分组，路由前缀 `/apps/<app-id>/<page-id>`。
4. 页面由 `AppPageRenderer` 渲染，组件白名单固定。
5. 权限沿用现有模型：应用页面声明 `permission: app.<id>:read`，由 Core 在安装时写入权限目录，角色授权仍走 `role-presets`。

这样 WebUI 只多一个渲染器和一个数据源，之后再加一百个应用也不改 WebUI。

复杂交互的逃生舱：如果某个应用确实需要自定义交互，允许 `ui.pages[].kind: sandboxed-web`，内容是一个签名过的静态 bundle，跑在同源 iframe 里，CSP 只允许自身资源，父子只通过 postMessage 传声明式消息，且必须声明 `ui.sandbox` 能力。这是个例外通道，默认关闭，安装时需要管理员确认。没有这个逃生舱，迟早会有人把整套前端塞进 layout 里。

## 10. 北向怎么不改

北向端口 `peek` 从 DataHub 读，`name` 和 `resolve_name` 查点位元数据。应用点位的处理方式是把它们**注册进同一张点位元数据表**，而不是让北向认识应用。

具体做法：应用激活时，宿主把 `telemetry.points` 和 `northbound.metrics` 合并进运行期点位表，带 `owner: app:<id>` 和 `lifecycle: app` 标记。北向已有的遍历逻辑自然就能看到它们；卸载时按 owner 一次性撤销。

要改的只有一处：点位元数据从静态数组变成一个可增删的注册表。这是 L0 内核的改动，一次完成，之后所有应用免费获得北向上送能力。

下行同理。应用若声明 `northbound.metrics[].rw: rw`，北向下行会先按 metric 名解析到 tag，再由宿主投递给应用。应用不接触 MQTT 或 104 的连接。

## 11. 数据、命名与配额

- 应用点位统一前缀 `app.<app-id>.`，与产品点位在同一张表但命名空间隔离，避免冲突。
- 每个应用有存储配额，KV 计入 `limits.storageKb`，超限写失败并告警。
- 每个应用有 CPU 时间配额（每秒毫秒数），超限先降频，持续超限按故障策略处理。
- 应用之间不能互读点位，除非双方都声明并且管理员在安装时确认。默认隔离。

## 12. 生命周期与版本

```
未安装 ──安装──> 已安装 ──启动──> 运行中 ──停止──> 已停止
                    │                 │
                    └──卸载──> 未安装  └──故障──> 已故障（重启或隔离）
```

- 安装：校验签名、schema、sdk 兼容、能力、配额；写入包；**注册贡献**；状态变为已安装。注册是原子的，任一步失败就回滚，不留半套贡献。
- 启动：调用 runtime `load` + `start`，开始接收点位事件。
- 升级：新版本先并存安装，贡献以新版本为准；健康检查通过后切换，失败回滚到旧版本。旧版本保留一个版本用于回滚。
- 卸载：停止运行时；按 `app-id` 撤销全部贡献；删除包；保留一段时间的 KV 供恢复。
- 开机恢复：沿用 `edge_app_mgr_boot_restore`，但恢复前要重新校验贡献是否与当前宿主版本兼容。

## 13. 安全模型

- 包必须签名，离线设备用内置公钥验签，签名为 Ed25519。
- 能力声明是唯一授权来源；运行时不提供任何未声明的宿主函数。
- 所有写操作（tag.write、alarm.raise、安装、升级、卸载）进现有审计日志，记录 app-id、版本、操作者。
- WASM 内存隔离；JS 与 Python 用进程或受限解释器隔离，故障按 `fault_policy` 自动重启或隔离。
- 应用不能访问网络，除非声明 `net.egress` 能力并配置白名单；现场多数场景应默认拒绝。

## 14. 迁移路径

分四步，每步都能独立验收。

**P0 契约冻结。** 定义 `app.yaml` v1、六个贡献类型的 JSON Schema、Host ABI v1（WIT 形式）、注册表 API。产出是 schema 文件、WIT 包和一份 conformance 用例集。验收：三份示例清单通过校验，两份错误清单被拒；C 与 Go 两个绑定能通过同一套 ABI 用例。

**P1 统一注册表。** 把网关现有的 `ui_configs` 联邦升级为带类型和版本的注册表；点位表从静态数组变成可增删注册表；WebUI 增加 `AppPageRenderer` 和一个动态"应用"分组；北向改为读注册表。验收：手工插入一个贡献，WebUI 和北向都能看到，无需改代码。

**P2 WASM 宿主。** 落地 runtime 适配层、Host ABI、配额、故障策略，把现有 `edge_app_mgr` 的清单扩展成 `app.yaml`。验收：示例应用能读点位、抛告警、贡献页面；超时和超内存按策略处理。

**P3 JS / Python。** 加 QuickJS 与 MicroPython 适配器，跑同一套 conformance 用例。验收：同一份应用逻辑在三个运行时行为一致；Python 适配器的常驻内存有实测数字。

**P4 生态与商店。** 签名、升级回滚、商店后端、`app.yaml` 校验工具、开发者文档。验收：从商店安装到运行到升级到卸载全链路走通，且全程没有改任何子系统代码。

## 15. SDK 分发：一套 ABI，N 个生成绑定

不是"每种语言手写一个 SDK"。手写 N 份必然漂移，最后每个运行时的语义都不一样。正确做法是**一份 IDL，生成 N 份绑定**。

- 唯一事实来源是一个 WIT 包，定义 Host ABI：函数名、参数、返回、错误、能力查询。
- C、Rust、Go 用 `wit-bindgen` 生成绑定；Python 用 `componentize-py`；JS 用 `jco` 或一个薄 QuickJS 适配层。
- 绑定是生成物，不进人工评审，不进手工修改。改 ABI 只改 WIT，然后重新生成。
- WIT 包的语义化版本就是清单里的 `sdk: 1.0`。主版本变化才允许破坏兼容，宿主在安装时拒绝不兼容的包。
- 每个绑定都要跑同一套 conformance 用例。绑定和运行时是两件事，两个维度都要覆盖：C 绑定配 WASM 宿主，Go 绑定配 sidecar 宿主，结果必须一致。

这样"支持 Go"在 SDK 层只剩一次生成，而不是一套长期维护的实现。

## 16. 进程模型：抄 VS Code 的边界，不抄它的单进程拓扑

VS Code 的结构是渲染进程、扩展宿主进程、每个 webview 各自隔离，渲染层从不加载扩展代码，两边只走 RPC。这层边界必须抄。但 VS Code 默认把所有扩展放在**同一个**扩展宿主进程里，一个失控扩展会拖住全部。桌面场景可以接受，网关不能接受。

所以进程模型按运行时分级，而不是按应用分级：

| 档位 | 适用 | 隔离方式 | 单实例成本 | 启动 |
| --- | --- | --- | --- | --- |
| A 进程内 WASM | C / Rust / Zig / TinyGo，事件驱动的小应用 | 线性内存隔离，配额限死 | 应用堆上限加运行时开销，通常几百 KB | 微秒级 |
| B 每应用 sidecar 进程 | Go / Python / Node，需要原生库、线程、socket | 进程隔离，unix socket RPC | 语言运行时自身，通常十几 MB | 百毫秒级 |
| C 同运行时共享 sidecar | 只在内存极紧的设备上，显式开启 | 共享进程，故障会互相影响 | 均摊 | 一次 |

规则是三条。

1. **应用代码永远不在网关主进程里跑。** 这条没有例外。
2. **ABI 与传输方式解耦。** 同一个 Host 函数，进程内是直接调用，sidecar 是 RPC。应用代码不感知差别，SDK 也不分叉。
3. **档位由运行时和资源画像决定，不由开发者随便选。** 清单里写 `runtime: go` 就进 B 档，写 `runtime: wasm` 就进 A 档，宿主按配额和档位做准入。

对 Go 的具体结论：**Go 应用默认走 sidecar，不默认编译成 WASM。** 理由是实测的体积差，见下一节。Go 编 WASM 在功能上可用，但对一个每次事件只做几次比较的应用来说，随包携带的 GC 运行时太重。

## 17. 实测：一个最小应用的体积差

同一个逻辑（读一个点位，越限时抛告警），在本机实测（Go 1.25.5，clang 自带 wasm-ld）：

| 实现 | 产物 | gzip |
| --- | --- | --- |
| C，`--target=wasm32 -nostdlib -O2` | 281 B | 248 B |
| Go，`GOOS=wasip1 GOARCH=wasm -ldflags="-s -w"` | 1,642,824 B | 492,130 B |

差约 5,800 倍。Go 版已经剥符号，仍然带完整 GC 与调度器。网关若有 64MB 应用预算，进程内 WASM 能放几十个 C/Rust 应用，却只能放约 40 个 Go 应用，而且每个都带一份独立运行时，内存碎片和 GC 开销会叠加。

未实测、需要补的：Rust `wasm32-wasip1` 的体积（本机没装该 target）、TinyGo 的体积与并发行为、`componentize-py` 的 Python 产物体积、sidecar 的 RPC 往返延迟与冷启动时间。这些数字应该在 P2 阶段用同一套方法测出来，再定档位阈值。

## 18. 反例与取舍

**不做 VS Code 式特权扩展。** 让应用在宿主进程跑任意 JS，等于放弃隔离，现场设备不认。

**不做 VS Code 式单扩展宿主。** 所有应用挤在一个进程，一个死循环就拖垮全部应用。这是桌面便利换来的代价，网关不付。

**不手写多语言 SDK。** 每语言一份手写 SDK 必然语义漂移。用 WIT 生成绑定，只维护一份契约。

**不做 iPhone 式自带 UI。** 每个应用一套前端，WebUI 会碎成十个风格，权限与审计也没法统一。声明式页面是唯一可持续的形态。

**不做 OSGi。** 动态加载 C 共享库在嵌入式网关上风险和体积都不划算，且有 ABI 稳定性问题。WASM 是更合适的隔离单元。

**不默认每应用一个微服务。** 每个应用一个进程在几十个应用时会吃光内存。所以 A 档走进程内 WASM，只有需要原生能力的语言才进 B 档，并且有进程数上限和 LRU 回收。

**不把注册表做成可执行钩子。** 贡献是数据，不是回调。一旦允许应用注册 C 回调，隔离就没了。

## 19. 风险与未决

- 声明式 layout 的表达力有上限。逃生舱（`sandboxed-web`）必须尽早设计，否则会被绕过。
- 动态点位进入北向，需要明确"应用卸载后历史点位怎么处理"，建议保留只读历史、停止上送。
- Python 运行时体积与启动时间需要实测，可能只适合少数应用，或改为按需启动。
- 安装时的贡献校验必须严格，否则一个坏清单可以污染全局点位表。建议注册表只接受经过 schema 校验的对象，拒绝任何自由字段。
- 应用的权限点（`app.<id>:read`）如何进入角色预置，需要和现有 `role-presets.yaml` 的构建期展开机制对齐。建议安装时生成，授权仍由管理员显式授予，不自动继承。
- sidecar 档位需要进程数上限、健康检查和 LRU 回收，否则内存会被慢启动的应用吃掉。冷启动延迟尚无实测数据。
- WIT 与 `wit-bindgen` 的版本选择要锁定，生成物入 CI 校验，避免绑定与契约悄悄漂移。
