# ADR 0012: 应用平台宿主模型与点位承载

状态：Proposed（2026-11-09 评审，待冻结）。

## 上下文

`docs/design/APP_PLATFORM.md` 的评审需要先定四件互相依赖的事：主进程是谁、app-manager 放哪、应用点位的值怎么走、app-host 与主进程怎么通信。代码事实：

- 生产镜像只启动一个 C 产品进程（`docker/Dockerfile.prod:70` 的 ENTRYPOINT 是产品可执行文件）；`go/cmd/edge-gateway` 只在 `scripts/start_mesh.sh` 的 dev mesh 里出现，无认证、无类型/版本、仓库内无客户端。
- DataHub 的影子表是扁平的 `PipeMsg[]`；`datahub_shm_publish` 先用产品注入的 `map_fn` 把 `tag_id` 映射成槽位，**`map_fn` 返回 `<0` 时回退为 `idx = tag_id`**（`modules/edge_core/src/datahub_shm.c:159-179`）；WAL 重放同规则（`datahub.c:570-585`）。
- `edge_app_mgr` 没有被任何产品构造，`start/stop` 只改数据库状态，不加载 WASM。
- `console_rbac`/`console_audit` 有定义、无调用；`console_rbac_verify_two_man_rule()` 接受任意 6 位数字和四个硬编码口令。

## 决策

1. **目标产品是 DCU；主进程是 C 的 `dcu` 进程，不是 Go 网关。** Go 网关不进入 P0 依赖链。
2. **app-manager 是产品进程内的模块**，不是独立进程；它是注册表唯一写者，持有 grant 表与 `grant_generation`，并承担授权最终判定。app-host 是独立进程，由产品进程 `posix_spawn` 拉起；不单独实现 supervisor。
3. **应用点位用"注册表名字 + DataHub 槽位"承载。** 注册表拥有 `app.<app-id>.<point-name>` 并分配相对槽位；产品进程计算 `app_tag = app_base + slot`；产品 `map_fn` 对 `tag_id >= app_base` 必须返回 `<0`，让 DataHub 回退为 `idx = tag_id`。槽位可回收，历史不按槽位存（历史键为 `(app_id, point_name, contract_version)`）。
4. **app-host 与产品进程之间用继承的 `socketpair(AF_UNIX, SOCK_SEQPACKET)`**，产品进程在 `posix_spawn` 前创建、把一端 `dup2` 给 app-host；每实例令牌随帧发送；`point_resolve` 返回 `(handle, grant_generation)`，每次调用复查代次，撤销后旧句柄返回 `DENIED_REVOKED`。
5. **运行态不进注册表。** `restarting`/`faulted`/`isolated`/重启次数留在产品进程内存，经状态接口读取；只持久化一张很小的故障连击表（NORMAL 同步，仅在故障跃迁时写）。

## 后果

- DataHub 的结构、`PipeMsg` 64 字节契约与 Go SHM 镜像都不改；只改容量常量与产品 `map_fn` 的让位规则。Go 侧容量硬编码（`edge-api-core` 1024、`smart-box-gateway` 16384）必须先与 C 侧同步，否则高槽位不可见。
- 应用点位的动态命名空间仍需一份 ADR-0010 修订说明；本 ADR 只决定实现形状，不替代那份修订。
- 11 号文档已按本 ADR 重写；01/02/04/05/06/07/10 已同步修订。
- app-manager 用 C 写在产品进程内，代价是管理面复用 C 的 JSON/SQLite 代码；若改为 Go 独立进程，必须重新引入跨进程 grant 同步，本 ADR 不覆盖该变体。
- A 档仍是协作式隔离：app-host 持有所有实例令牌，可冒充实例，但绕不过产品进程的 grant 检查；高风险写能力只允许 sidecar。

## 备选（未采用）

- **DataHub 扩展动态槽位**：需要改影子表布局、PipeMsg 语义与 Go 镜像，P0 风险最大，收益要到 UI/北向接入才兑现。
- **产品侧独立值表**：P0 更直白，但 UI/北向接入时会出现两条值路径，最终仍要迁回 DataHub。
- **独立 app-manager 进程 + Go 网关为主进程**：与生产部署不符，且授权判定要跨进程同步，撤销 200ms 目标更难保证。
