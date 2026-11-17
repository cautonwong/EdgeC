# 12 执行模型：数据流管道与应用运行时

状态：决定记录（2026-11-15）。本文件只固定运行时边界，不新增功能。

## 1. 结论

EdgeC 有两个执行面，职责不重叠。

| 执行面 | 实现 | 负责 | 不负责 |
|---|---|---|---|
| 数据流管道 | `modules/edge_stream` | 产品点位上的过滤、变换、聚合、落库、导出 | 应用安装、授权、生命周期 |
| 应用运行时 | `cmd/app-host` + `modules/edge_app_host` | 单个 WASM 应用的加载、Host ABI 转发、隔离与配额 | 产品数据流编排、直接访问 DataHub/SQLite |

不新增第三个执行面。具体来说，不实现独立的 EdgeX 风格 App Functions SDK，也不引入 `pipe_fn(ctx, in, out)` 这套并行插件 ABI。

## 2. 数据流管道

现有能力已经覆盖管道式数据处理：

- `modules/edge_stream/src/pipeline_loader.c` 按 JSON 声明装配 stage。
- `standard_stages.c` 的 `g_standard_stages[]` 当前注册 38 个 stage，覆盖 filter、filter_range、transform、ema、clamp、peak_shave、accumulate、compactor、rbe、moving_average、sqlite_sink 等。
- `stream_engine_register_stage_factory` 允许产品在组合根注册自定义 stage；`stage_wasm` 和 `js_stage` 已经提供受限的自定义代码入口。
- 管道的输入是产品点位 tag，输出通过 `stream_bus_port_t` 或北向驱动离开进程。

管道的所有权属于产品配置，不属于应用包。应用不能直接改产品管道。

## 3. 应用运行时

`app-host` 是唯一的第三方应用运行时：

- 一个进程只加载一个 WASM 实例，通过继承的 socketpair 与产品通信。
- 只暴露 WIT `app.v1` 定义的 Host ABI：`point-resolve`、`point-read`、`point-subscribe`、`point-publish`、`log`、`now`，以及 `app-init`、`app-on-event`、`app-stop` 三个导出。
- 不链接 DataHub 和 SQLite；所有数据访问经 `app_host_ports_t` 由组合根注入。
- `point_subscribe` 与 `app_on_event` 的投递链路已实现：订阅、每应用事件队列、ack、keepalive、授权复查与平台事件（`grant_changed`、`settings_changed`、`app_will_stop`、`tick`）都在宿主完成。

## 4. 为什么不做 App Functions SDK

1. **重复已有引擎。** 38 个注册 stage、JSON 装配、自定义工厂和北向驱动已经在仓库里。另起一套 SDK 会复制同一批职责。
2. **第三套生命周期。** 现在已有产品管道配置和 WASM 应用生命周期。EdgeX 风格 SDK 会引入第三套打包、权限、插件 ABI 和错误模型。
3. **没有真实消费者。** 仓库里没有 EdgeX 应用迁移清单，也没有客户要求运行 EdgeX app functions。先造 SDK 再找应用，不符合最小改动。
4. **权限模型会分叉。** 应用点位的授权走 WIT Host ABI 和注册表；管道如果自带触发器与导出，会绕开这套逐次校验。

## 5. 如果应用需要管道

未来如果出现“应用自带管道”的真实需求，路径是把应用清单里的 `contributes.pipelines` 映射到现有 `pipeline_loader`，而不是新增 SDK：

- 管道仍由 `edge_stream` 执行，stage 仍来自 `g_standard_stages[]` 或组合根注册的工厂。
- 应用只能引用已授权点位，不能新增触发器类型或导出协议。
- 新增的只是清单字段、校验和装载逻辑，不新增运行时。

这条路径当前没有实现。在第一个真实应用出现前，不为它加字段或抽象。

### 5.1 自定义 stage 的扩展方式

产品在组合根调用 `stream_engine_register_stage_factory("type", factory)`，管道 JSON 用 `{"type":"type","config":{...}}` 引用。工厂拿到 `cJSON` 配置，返回一个 `pipeline_stage_t`；不需要改 `pipeline_loader.c` 或核心引擎。

`tests/test_stage_factory_extension.c` 用一个 `test_scale` 工厂加一个 `test_capture` 工厂验证了这条路径：JSON 装载、stage 串联、数据变换都通过公共接口完成。

## 6. 边界规则

- `modules/edge_stream` 不得包含应用安装、授权、进程管理相关头文件。
- `modules/edge_app_host` 和 `cmd/app-host` 不得包含 `datahub.h`、SQLite 头文件或 `stream_engine_priv.h`。
- `sdk/` 只做线协议编解码和导出分发，不实现业务逻辑，不直接访问 DataHub。
- 新增执行面必须先改本文件，再写代码。

## 7. 未决

- 平台事件已实现 `grant_changed`、`settings_changed`、`app_will_stop` 与 `tick`；`upstream.*`、`registry.generation`、`action.invoke` 与完整信封 v2 仍未实现。
- 应用清单的 `contributes.pipelines` 只是未来路径，没有字段、schema 或验收。
- 管道的自定义 stage 是否能由应用包提供，取决于沙箱和授权模型，当前不允许。
