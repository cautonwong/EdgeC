# EdgeOS WebUI 平台

一套 Core 支撑多个产品（iEMS / DCU / SmartBox），每个产品只写自己的 **Product Pack**。
设计依据：[`docs/边缘网关_WebUI平台详细设计文档_v1.6_最终设计基线.md`](./docs/边缘网关_WebUI平台详细设计文档_v1.6_最终设计基线.md)。

## 1. 这一层解决什么问题

旧实现是"契约驱动 UI"：前端启动后从 `/api/v1/contracts` 拉点位表，按 `ui.group` 现场拼菜单。
它有三个结构性问题，重构就是为了消掉它们：

| 旧问题 | 现在的做法 |
| --- | --- |
| 菜单由运行期数据决定 → 权限/能力无从表达 | 菜单由**构建期生成的静态 Registry** ∩ 用户权限 ∩ 设备能力 ∩ 功能许可决定 |
| 每个产品各写一套 Shell、权限各写一遍 | Core 提供 Shell / 授权呈现 / 组件库；Pack 只声明页面、权限引用与主题 |
| 前端隐藏被当成安全边界 | 每个 API 由服务端逐请求判定（权限 + 范围 + 能力 + 许可），前端只影响呈现 |

## 2. 目录与分层

```
web/
├─ core/                        # @edgeos/web-core（npm workspace，Pack 唯一依赖方向）
│  ├─ modules.manifest.yaml     # Core 模块与页面清单（构建输入）
│  └─ src/
│     ├─ sdk/                   # 类型 + 纯逻辑：权限合成、页面可见性、Registry 自检、请求层
│     ├─ ui/                    # Design Token（三层）+ L1/L2 组件
│     ├─ app/                   # App Shell：登录/改密、路由守卫、stores、布局、配置事务条
│     └─ modules/               # Core 通用页面（由旧 core/views 迁入，逐步下沉到 Pack）
├─ products/
│  ├─ iems/  product.yaml · release-scope.yaml · openapi.extension.yaml · pages/
│  ├─ dcu/   同上
│  └─ smart_box/                # 仍是独立前端（未接入平台），保留原构建
├─ permissions/{catalog,role-presets}.yaml   # 权限目录与角色预置（唯一事实来源）
├─ schemas/*.json               # 配置 Schema：字段/风险等级/写权限/确认窗口（前后端共用）
├─ openapi/core.yaml            # HTTP 契约 + operation 级安全元数据（唯一事实来源）
├─ tooling/                     # 构建期：manifest 校验 / Registry 生成 / 边界 / 对比度 / 体积
├─ mock-server/                 # webd 的行为替身：真实做授权、会话、事务状态机
└─ tests/e2e/                   # Playwright（legacy/ 为迁移期历史用例，已排除）
```

依赖方向（`tooling boundaries` 强制，违反即构建失败）：

```
Product Pack ──→ core/sdk、core/ui            （入口 main.ts 额外可用 core/app 的 createProductApp）
core/app     ──→ core/sdk、core/ui、core/modules
core/ui      ──→ core/sdk
core/sdk     ──→ 无框架依赖（不得 import vue / pinia / axios）——所以它能在 node 里直接单测
Core 永不 import products
```

## 3. 构建期发生了什么

`npm run build`（每个产品）= `tooling registry` + `vite build`：

1. **校验构建输入**：`product.yaml`、`release-scope.yaml`、Core 模块清单、权限目录、OpenAPI。
   悬空权限、重复 ID、未登记的一级分组、缺失的页面组件、匿名接口未进 allowlist —— 全部构建失败。
2. **裁剪**：功能是否出现 = `product.yaml` 声明 **AND** `release-scope.yaml` 启用。
   任一关闭即不生成页面 / 接口 / 路由，产物里不留"隐藏的入口"。
3. **生成静态 Registry**（`src/generated/registry.json`）：路由、权限、角色（通配符已展开）、
   能力需求、API 策略表、release-scope 摘要与 Registry 摘要。同一输入必然得到同一字节。
4. **vite build**：页面按 chunk 懒加载；Registry 决定"哪些 chunk 存在"，运行期只做过滤。

## 4. 配置事务（R0–R5）

`schemas/<sectionId>.json` 是配置页的唯一事实来源：字段、校验、**风险等级**、写权限、
确认窗口都写在里面。构建期校验它（字段类型/format、riskClass 与窗口是否匹配、写权限是否
在目录里），并把整份 schema 写进 Registry；服务端读同一份文件做校验与窗口决策。

```
编辑 → 应用 → （R2 起）PENDING_CONFIRM 倒计时 → 确认保留 / 超时由设备 watchdog 回滚
```

- 前端：`useConfigSection()` + `<UiSchemaForm>`（按 Schema 渲染控件）+ `<ConfigTransactionBar>`；
  前端校验只是提前提示，服务端 422 的 `fieldErrors` 才是结论，会挂回对应输入框。
- 倒计时只用于显示：关掉浏览器照样回滚，因为 deadline 与回滚在设备侧。
- 已接页面：网络接口（R2）、系统时间（R1，应用即生效）、防火墙（R3，含"默认拒绝 + 白名单为空"
  的锁定风险提示）。三个页面共用同一套表单/事务组件，页面里没有"if 这是 R1"这类分支——
  风险等级由 Schema 与事务条决定。

## 5. 实时通道（只读推送）

`GET /api/v1/ws` 是**只读**通道（§6.3）：客户端连上后只能 `subscribe` / `unsubscribe`，
服务端推送 `alarm` / `task` / `config-transaction` / `telemetry`。不引 `ws` 库——按 RFC 6455
手工实现最小子集（握手 + text/close/ping 帧 + 强制掩码校验），离线设备也装得下。

- 客户端 `sdk/live.ts`：指数退避重连；**重连成功会触发 `resync`**，因为服务端不保留积压，
  漏掉的消息由消费者整体重取，而不是假装连续。
- `stores/live.ts`：连接状态 + 最近事件 + 订阅分发；`seq` 不连续时主动要求重取。
- 顶栏徽标显示 `实时 / 重连中 / 离线`；配置事务条订阅 `config-transaction`，
  所以"别的窗口确认了"或"watchdog 超时回滚了"，本页不用手动刷新就能看到结果。
- 告警页（`/events`）订阅 `alarm` 主题：配置回滚、任务取消会真的产生告警并推到页面；
  确认告警是写操作（`core.alarm:write`），服务端同样判定。

## 5.0 概览与遥测（§8.1）

- 概览页数据面从「契约点位」换成 `GET /api/v1/telemetry/current` + WebSocket `telemetry` 主题：
  打开取一次全量，之后靠推送增量更新，**不轮询**。
- 超过 8 秒没有推送就把卡片标记为"已过期"——停更的数值看着像实时的，比没有更危险（§9.3）。
- 分组过滤与搜索走前端；点位数值由服务端用确定性正弦生成，E2E 能验"值真的在动"。

## 5.1 用户与角色（§5.5）

- `/system/users`：新建（返回**一次性口令**，只显示一次）、重置口令（踢掉该用户全部会话）、
  禁用/启用、删除。三条不变量前后端一致：不能禁用/删除自己（`self_protected`）、
  不能禁用或降级最后一个可用管理员（`last_admin`）、重置口令强制下次改密。
- `/system/roles`：内置角色只读（权限来自构建期 role-presets）；自定义角色在设备上创建，
  权限从 Registry 的目录里逐条勾选，**必须是具体 ID**，且不能超过自己的权限（`grant_escalation`）。
- 台架账号：`opsadmin / OpsAdmin-Pass-2026`（管理员，用于用户管理；`admin` 专门留给首登改密流程）。

## 6. 常用命令

```bash
# 构建输入与规则检查（CI 里也跑这一条）
node tooling/cli.mjs all          # 校验 + 依赖边界 + 对比度 + 为所有产品生成 Registry
node tooling/cli.mjs validate
node tooling/cli.mjs boundaries
node tooling/cli.mjs contrast     # Design Token 语义配对对比度（WCAG）
node tooling/cli.mjs schemas      # 配置 Schema 一览（风险等级/窗口/字段）
node tooling/cli.mjs registry --product iems
node tooling/cli.mjs size --product iems

# 单测（不需要浏览器）
npm run test:unit                 # tooling 用例 + core/sdk 纯逻辑（tsc 编译后跑 node:test）

# 本地跑起来
node mock-server/server.mjs --product iems --port 8081
npm run dev --prefix products/iems        # http://127.0.0.1:5173，/api 代理到 8081

# E2E（真实浏览器 + mock-server）
npm run test:e2e
```

种子账号（mock-server，仅用于台架）：

| 账号 | 口令 | 用途 |
| --- | --- | --- |
| admin | `Initial-EdgeOS-2026` | 首登强制改密流程 |
| engineer | `Engineer-Pass-2026` | 现场调试角色 |
| viewer | `Viewer-Pass-2026` | 只读 |

## 7. 几条不能破的规则

- **前端隐藏不是授权**：`PermissionGate` / 菜单过滤只影响体验；写接口一律服务端判。
- **能力未知 = 不支持**（fail-closed）：能力快照拿不到时，需要能力的页面与接口都不放行。
- **运行期不注册路由**：路由只能来自构建期 Registry；服务端不能临时加外部路由。
- **生成物不入库**：`products/*/src/generated/` 由 tooling 生成，禁止手改。
- **通配符只活在构建输入**：Registry 与运行时请求里只有具体权限 ID。

## 8. 边界（哪些还没做）

如实列出，避免"看着像做完"：

- **webd 本体**（Go/C++ 常驻服务、TLS、IPC、审计持久化、任务执行器）未实现；
  `mock-server` 只覆盖 HTTP 语义与授权判定，不能当作设备验证。
- **配置事务的独立 watchdog**（跨重启恢复、kill -9 演练）未实现；当前只有状态机、deadline 与回滚。
  另外"刷新页面恢复未确认事务"目前按当前用户查列表实现，不是设备侧推送。
- **WebSocket** 已接事务/任务/告警/遥测四类推送源；仍没有"断线期间的事件回放"
  （服务端不保留积压，靠 resync 让消费者整体重取）。
- 遥测订阅目前是**全局**的（登录即订阅），8 个点位 2s 一次没问题；点位规模上来后要改成
  按页面订阅，否则每个页面都在收自己用不到的数据。
- **告警页**替换了旧 EventStreamHub 的数据面（旧组件文件仍在 `modules/events/`，不再挂路由）；
  告警的"抑制/归并/通知渠道"未做。
- **旧页面数据面**仍走 `/api/v1/contracts`（标记 deprecated），随 WBUI-072/074 收敛到 Pack API。
- **Pack 布局**沿用 `core` + `products/*` 两个 workspace（设计文档写的是 `packages/*`）：
  改成 `packages/*` 需要重装依赖并重生成 lockfile，留作独立任务，不影响本 PR 的分层与门禁。
- 旧页面历史 E2E 用例在 `tests/e2e/legacy/`，**未修复也未删除**，明确排除在运行之外。
