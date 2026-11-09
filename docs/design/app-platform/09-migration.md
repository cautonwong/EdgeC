# 09 迁移与废弃

状态：设计基线，待评审。未实现。

## 1. 要迁移的四样东西

新平台不是在一张白纸上开始。仓库和现场已经有四样东西与它重叠，必须给出处置，否则"拒绝旧模块"会直接打断现场。

| 现有物 | 位置 | 与新平台的关系 |
| --- | --- | --- |
| 进程内 WAMR 运行时 | `modules/edge_stream/src/wasm_runtime` | 新平台要求应用跑在 app-host，不在产品进程 |
| 旧 `env` ABI 与示例 | `docs/wasm_micro_app_guide.md`、`examples/wasm/*` | 全局数字 TagID，与 ADR-0010 冲突 |
| QuickJS 脚本引擎 | `modules/edge_stream/src/js_engine.c`、`js_stage.c` | 新平台计划用 QuickJS，不能另起一套 |
| 应用清单与已装应用 | `edge_app_mgr` 与 `app_registry` 表 | 字段与状态机需要扩展和转换 |
| 应用商店静态目录 | `web/core/src/modules/apps/catalog.ts`（PR #512） | 安装状态写 localStorage，应改为消费注册表 |

**现状修正（2026-11-09）。** `edge_app_mgr` 没有被任何产品构造（只有测试），`start/stop` 只改数据库状态、不加载 WASM；真正在运行的应用路径是 IEMS 的 REST 上传 → `stage_wasm` pipeline。因此"盘点已装应用"很可能盘出空表，迁移工具要先扫 `stage_wasm` 与 `app_registry` 两条路径，而不是只扫 `edge_app_mgr`。

## 2. 旧 WASM 运行时

**目标状态。** 应用只在 app-host 进程里运行，产品进程不加载应用代码。进程内的 `wasm_mgr` 不再接新应用。

**迁移路径（2026-11-09 已定）。**

1. **冻结入口**：`edge_app_mgr_install` 拒绝新安装；IEMS 的 `POST /api/v1/apps/upload` 改收 `.eapp`，验签后交给产品进程的 app-manager，旧 raw wasm 直接 `DENIED_ABI`，不再进 `stage_wasm`。app-manager 成为唯一安装入口，否则授权与审计永远有旁路。
2. **只读期**：`stage_wasm` 只允许已安装的旧模块继续运行，期限为 6 个月或 2 个固件发布周期（取更长者）；期间阻止新上传与新启动，页面标注"旧运行时"。
3. **盘点**：工具扫 `stage_wasm` 与 `app_registry` 两条路径，输出应用名、版本、数字 TagID、是否可以重打包；清单为空之前不执行移除。
4. **迁移**：现场逐个重打包为新 ABI 并安装；工具给出旧应用的清单、点位、触发条件。
5. **移除**：只读期结束后移除进程内路径与相关代码，保留一个发布周期的回滚分支。

**smart_box 的 `rehab_companion`（2026-11-09 已定）。** 重打包为第一方 `.eapp`，不再在产品代码里硬编码路径与文件名；若赶不上 P0，保留硬编码并明确标注例外，不假装已迁移。

**回滚。** 旧运行时二进制保留一个发布周期；数据库在迁移前做快照。回滚要求旧快照与旧二进制同时可用，不能只回滚一半。

## 3. 旧 `env` ABI

旧 ABI 是模块 `"env"` 加 `ext_datahub_read(1001, ...)`，点位是全局数字 TagID。新 ABI 是 `edgeos` 加命名点位。

**已定拒绝（2026-11-09）。** 未声明 `abi: 1` 的模块安装即拒，错误码 `DENIED_ABI`。理由：兼容层会把 ADR-0010 刚废除的全局数字标识重新引回业务路径；仓库没有真实第三方存量；iOS 与 VS Code 都采用硬性最低版本，而不是永久兼容层。

**迁移工具。** 一个离线命令，做三件事：

1. 扫描旧包，列出它读写的数字 TagID。
2. 按产品 ACL 把每个数字 ID 映射到点位名，生成 `app.yaml` 草案与 `requires.points`。
3. 生成新 ABI 的绑定骨架，旧代码的业务逻辑保留，只替换取数与发布调用。

映射不上或一个数字 ID 对应多个点位时，工具停下来要求人工指定，不猜。

**告警期。** 现场盘点完成前不切断旧模块。盘点结果是一个清单：应用名、版本、责任人、依赖的数字 ID、是否可以重打包。清单为空之前不执行第 2 节的移除步骤。

**盘点执行（2026-11-09 已定）。** 出厂前产线自动扫 `stage_wasm` 与 `app_registry`，输出 JSON 清单并写设备审计，空清单是出厂条件之一；固件升级前自动扫描，有旧应用则阻断升级并提示先迁移或卸载，不允许"升完再说"；现场可随时执行 `appctl legacy scan`，输出同样的 JSON。出厂前由产线质量负责，升级时由升级操作人负责；Edge-Ops 只读扫描结果，不代替执行。清单包含应用名、版本、数字 TagID、触发条件、是否可重打包、最后运行时间；空清单才允许 `legacy disable`。

## 4. QuickJS

仓库已有 `js_engine` 与 `js_stage`。**复用引擎，不复用宿主形态（2026-11-09 已定）**：

- **后装 JS 默认走 sidecar**：独立 QuickJS 进程，继承 B 档的 uid/cgroup/seccomp/socketpair；清单声明 `execution_profile: sidecar` + `runtime_engine: quickjs`。
- app-host 内的 `js_engine` 只保留给平台内部脚本与第一方诊断，不加载后装应用；`script_loader` 的热加载与定时执行同样只用于平台脚本。
- 第三方不得用 `shared` 跑 JS；A 档内的 JS 仅限第一方、签名受信、明确标注"仅故障隔离"。
- **JS 应用构建期打成单文件**：esbuild/rollup 把纯 JS 依赖打进 `app.js`，`node_modules` 不进包；原生插件与 Node 内置模块拒绝。运行时没有 `require` / `node_modules`，只有 `edgeos` 模块与标准 ES 子集。
- QuickJS sidecar 初值 16MB 堆、启动 p95 ≤100ms；补齐内存上限、中断回调、导入白名单后再开放。
- **QuickJS 沙箱（2026-11-09 已定）**：`JS_SetMemoryLimit(16MB)` + cgroup `memory.max=32MB`；`JS_SetInterruptHandler` 每 10ms 检查 deadline，超 10ms 抛 interrupt；`JS_SetMaxStackSize(1MB)`；保持无 JIT。模块白名单只允许 `edgeos` 与打包进 `app.js` 的代码，禁止动态 `import()`，`eval` 关闭或限制为无副作用；无 `fs`/`net`/`child_process`。超限/OOM 按 `fault_policy` 重启，不杀宿主。
- 迁移时把现有 `js_stage` 里的业务脚本按上述规则重打包；平台脚本保留。

## 5. 应用清单与数据

**字段扩展。** `app_registry` 增加 `package_digest`、`abi`、`desired_version`、`published_version`、`settings_schema_version`、`data_version`。旧字段保留一个周期，迁移完成后删除。

**状态映射。**

| 旧状态 | 新状态 |
| --- | --- |
| `UNINSTALLED` | `uninstalled` |
| `INSTALLED` | `installed` + `desired_version` |
| `RUNNING` | `active` + `published_version` |
| `STOPPED` | `installed` + `desired_version` 为空 |
| `FAULTED` | `installed` + 版本 `failed` |

**数据迁移（2026-11-09 已定）。** 同名同类型直接搬，int→number 等安全转换自动完成；同名异类型、一个旧键对应多个新字段、必填字段缺旧值这三类不猜，应用停在 `awaiting_migration_review`，不影响其他应用。`appctl migrate review <app>` 显示旧键→新字段、旧值→新值的 diff，管理员逐项接受或指定后才允许首次启动。迁移写 `migration_report` 与审计；迁移前参数快照保留到回滚期结束，回滚时原样恢复，不反向转换。迁移幂等，可重复执行。

## 6. 应用商店

已合并的商店页面（PR #512）用静态目录加 localStorage 记录安装状态。这与注册表的"管理员批准 + 运行时校验"冲突，必须改：

- 目录改为从 `/api/v1/apps/catalog` 读取，服务端由产品进程的注册表与可用包生成，`edge-web` 只做代理。
- 安装状态改为读注册表，不再写 localStorage。
- 安装按钮调用真实安装 API，进入 pending 授权流程；未批准时不能启动。
- 在真实 API 落地前，商店页面必须标注"演示数据"，不能让人以为安装已生效。

**catalog 与安装入口（2026-11-09 已定）。**

1. **包来源三类**：本地上传（Web / `appctl`）、U 盘目录扫描、Edge-Ops 分发的签名包缓存。P0 只做本地上传。
2. **catalog 由产品进程生成**：已安装应用（注册表）+ 可用包（扫描 `/var/edgeos/packages/*.eapp` + Edge-Ops 缓存）；每项带 `compatible` 标记（abi/sdk/products 校验结果）与不可用原因。
3. **上传走管理 API**：`POST /api/v1/apps/upload`，限制大小（初值 32MB），先验签再解包，写临时目录；失败返回可读错误码。上传不等于安装，管理员点"安装"才进入 `validate → stage → awaiting_grant`。
4. **catalog 不写 localStorage**；离线时显示"无法获取远程目录"，本地包仍可装。

### Go 侧处置（2026-11-09 已定）

- **删除**（迁移完成后）：`edge-gateway` 的内存注册表与 `/api/v1/internal/registry`、`/api/v1/discovery` 标签联邦；`edge-api-core`（SSE 与影子表读取并入 `edge-web` 的产品 API 代理）；`ui-backend`（并入 `edge-web`）；`demo_shm`、`demo-algo`（移出生产镜像）。
- **冻结**：`go/pkg/shm` 不再新增调用方（容量镜像 1024/16384 的分叉就是反例）；`go/pkg/tags` 只保留给 dev 工具，运行时元数据从产品 API 取。
- **保留**：`edge-web`（新）；`edge-ops`（云端面）；`smart-box-gateway`（DoIP/UDS 产品适配器，冻结新功能，迁移到 `edge-web` 会话后再评估）。
- **不新增**：任何在 Go 侧再建一份注册表、点位表或权限判定的组件。

## 7. 分阶段与回滚

| 阶段 | 动作 | 退出条件 |
| --- | --- | --- |
| M0 盘点 | 扫描旧包、旧应用、数字 ID 映射 | 清单完整，责任人确认 |
| M1 冻结 | 停止新装旧模块，保留查询与卸载 | 无新增旧模块 |
| M2 双跑 | 新平台可用，旧应用只读期 | 迁走一个真实应用并稳定运行 |
| M3 切流 | 新装只走新平台 | 旧应用只剩只读 |
| M4 移除 | 删除进程内运行时与旧 ABI 路径 | 一个发布周期无回滚请求 |

每个阶段都要有回滚点：M1 前无破坏；M2 可以切回旧运行时；M3 保留旧数据库快照；M4 之前确认没有旧应用。

## 8. 验收

1. 旧模块在新平台安装被拒，错误码与迁移提示可读。
2. 迁移工具能扫描旧包并列出数字 ID 与映射结果。
3. 映射失败时工具停下并要求人工指定，不生成猜测结果。
4. 迁移后的应用在新平台运行，读到的点位值与旧版一致。
5. 旧应用在只读期仍可查询与卸载，不能新装。
6. 旧状态映射后，页面显示与迁移前一致。
7. 商店页面不再写 localStorage，安装状态来自注册表。
8. 在 M2 回滚到旧运行时，旧应用继续工作。
9. 移除旧运行时后，回滚分支仍可在一个发布周期内使用。
10. 盘点清单为空之前，M4 不允许执行。

## 9. 未决

- 只读期与回滚期已定（2026-11-09）：6 个月或 2 个固件发布周期取更长者；到期硬移除，管理员可在盘点清单为空后 `appctl legacy disable` 提前结束，需审计。
- `js_engine` 加沙箱的改动量尚未评估。
- 旧应用的参数迁移已定（2026-11-09）：自动映射、歧义阻断、`migrate review` 人工确认、回滚快照，见 §5。
- 现场盘点执行已定（2026-11-09）：出厂前必跑、升级前阻断、`appctl legacy scan`，产线/升级操作人负责，见 §3。
