# EdgeOS WebUI 平台

一套 Core 支撑多个产品（iEMS / DCU / SmartBox），每个产品只写自己的 **Product Pack**。
设计依据：[`边缘网关_WebUI平台详细设计文档_v1.6_最终设计基线.md`](./边缘网关_WebUI平台详细设计文档_v1.6_最终设计基线.md)。

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
│     ├─ app/                   # App Shell：登录/改密、路由守卫、stores、布局
│     └─ modules/               # Core 通用页面（由旧 core/views 迁入，逐步下沉到 Pack）
├─ products/
│  ├─ iems/  product.yaml · release-scope.yaml · openapi.extension.yaml · pages/
│  ├─ dcu/   同上
│  └─ smart_box/                # 仍是独立前端（未接入平台），保留原构建
├─ permissions/{catalog,role-presets}.yaml   # 权限目录与角色预置（唯一事实来源）
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

## 4. 常用命令

```bash
# 构建输入与规则检查（CI 里也跑这一条）
node tooling/cli.mjs all          # 校验 + 依赖边界 + 对比度 + 为所有产品生成 Registry
node tooling/cli.mjs validate
node tooling/cli.mjs boundaries
node tooling/cli.mjs contrast     # Design Token 语义配对对比度（WCAG）
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

## 5. 几条不能破的规则

- **前端隐藏不是授权**：`PermissionGate` / 菜单过滤只影响体验；写接口一律服务端判。
- **能力未知 = 不支持**（fail-closed）：能力快照拿不到时，需要能力的页面与接口都不放行。
- **运行期不注册路由**：路由只能来自构建期 Registry；服务端不能临时加外部路由。
- **生成物不入库**：`products/*/src/generated/` 由 tooling 生成，禁止手改。
- **通配符只活在构建输入**：Registry 与运行时请求里只有具体权限 ID。

## 6. 本 PR 的边界（哪些还没做）

如实列出，避免"看着像做完"：

- **webd 本体**（Go/C++ 常驻服务、TLS、IPC、审计持久化、任务执行器）未实现；
  `mock-server` 只覆盖 HTTP 语义与授权判定，不能当作设备验证。
- **配置事务的独立 watchdog**（跨重启恢复、kill -9 演练）未实现；当前只有状态机与 deadline。
- **WebSocket 实时通道**未实现（契约里已登记 `GET /api/v1/ws`，服务端返回 426）。
- **旧页面数据面**仍走 `/api/v1/contracts`（标记 deprecated），随 WBUI-072/074 收敛到 Pack API。
- **Pack 布局**沿用 `core` + `products/*` 两个 workspace（设计文档写的是 `packages/*`）：
  改成 `packages/*` 需要重装依赖并重生成 lockfile，留作独立任务，不影响本 PR 的分层与门禁。
- 旧页面历史 E2E 用例在 `tests/e2e/legacy/`，**未修复也未删除**，明确排除在运行之外。
