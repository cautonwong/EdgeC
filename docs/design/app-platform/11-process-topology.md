# 11 进程拓扑与可信计算基

状态：第二轮修订（2026-11-09/10），待复审。未实现。

修订要点：主进程从"Go 网关"改为**产品进程**（P0 为 `dcu`）；app-manager 并入产品进程；不再有独立 supervisor；app-host 与产品进程之间用继承的 `socketpair`，不用文件系统 socket 路径。

## 1. 拓扑

| 进程 | 职责 | 与谁通信 | 身份与授权 |
| --- | --- | --- | --- |
| 产品进程（P0：`dcu`） | 产品 ACL、DataHub 影子值、北向、采集、**app-manager**、状态接口。**授权最终强制点** | app-host（继承 fd）、appctl（unix socket） | 持有 grant 表与 `grant_generation`，对每次调用做最终判定 |
| app-host | 承载 N 个 WASM 实例，有界 worker 池调度 | 产品进程（一条 `socketpair`） | 每实例令牌；由产品进程 `posix_spawn` 拉起 |
| appctl | 管理命令行（安装、批准、启停、查看） | 产品进程 | 本机管理员账号 |
| sidecar（B 档，P0 不实现） | 承载 Go/Python 应用 | 产品进程（每应用继承一条 socketpair） | 每应用 uid；继承 fd 为主，外部拉起的例外用 `SO_PEERCRED` |
| Edge-Ops（可选） | 车队策略下发、签名包分发 | 产品进程（北向通道） | 设备证书；v1 不远程批准 grant，只下发策略与包 |

产品进程不加载任何应用代码。应用代码只在 app-host 与 sidecar 里。

**为什么不用 Go 网关做主进程。** `go/cmd/edge-gateway` 只在 `scripts/start_mesh.sh` 的 dev mesh 里出现；生产镜像的 ENTRYPOINT 是产品可执行文件（`docker/Dockerfile.prod:70`）。它没有产品 ACL、没有 DataHub 产品点位映射、注册接口无认证且仓库内无客户端。把授权强制点放在一个生产里不存在的进程上，F 号文档无法验收。

### Go Web 层（edge-web，非 P0）

现有三个 Go 服务（`edge-gateway`、`edge-api-core`、`ui-backend`）收敛为一个 `edge-web`，职责只有三件：

1. 服务构建好的静态 SPA。
2. 浏览器会话认证（本机账号，服务端会话 + CSRF），与产品进程的 grant 判定分开。
3. 把数据与管理请求代理到产品进程的本地接口（`/run/edgeos/mgmt.sock` 与只读数据 API）。

- **静态托管**：按产品配置选 `www/<product>` 根目录，SPA fallback 与原 `ui-backend` 一致；不重写前端、不重新打包。
- **会话契约**：沿用 SPA 现有的 `/api/v1/auth/login|logout|me|password` 与 CSRF 头，`edge-web` 用本机账号实现同一组端点。会话用服务端存储（控制库 `session` 表，token 哈希、角色、过期、CSRF secret），Cookie `HttpOnly; SameSite=Strict; Secure`；不用 JWT 签名密钥。撤销删行，改口令删该用户全部会话。
- **传输（2026-11-09 已定）**：`edge-web` 默认 443 HTTPS，设备证书出厂签发（厂商 CA），主机名 `edge-<serial>.local`；80 只做 301 跳转。`Secure` Cookie 因此在 HTTPS 下生效。仅当显式开启 `allow_insecure_http=true` 且绑定 `127.0.0.1` 时允许 HTTP 本机调试，会话只允许本机。HSTS 只在 HTTPS 下设置，`max-age` 初值 1 天。
- **顶层启动**：systemd（容器用最小 init）拉起产品进程与 `edge-web`；产品进程只 spawn app-host。`edge-web` 连不上产品进程时静态页面仍可服务，API 返回 503 并重试；产品进程不依赖 `edge-web`。
- **`registry.json`** 仍是构建期产物；应用页面是外壳里固定的通用路由（`/apps/:appId/...`），安装应用不触发前端构建。`permission_source` 合并构建期核心权限与注册表快照的应用权限。

`edge-web` **不持有注册表、不判定 grant、不直接映射 DataHub 影子表**。删除 `POST /api/v1/internal/registry`、`/api/v1/discovery` 的标签联邦与内存注册表；标签元数据从产品 API 取，影子表容量从配置/头读，不硬编码。`smart-box-gateway` 若因 DoIP/UDS 需要保留，作为产品专用适配器单独存在；`edge-ops` 是云端面，不并入。

参照：iOS 的 SpringBoard/WebKit 只负责呈现，权限判定在 securityd；VS Code 的 renderer 不决定扩展权限，extension host 才决定。Web 层是呈现层，不是权威。

**不单独实现 supervisor。** 产品进程 `posix_spawn` app-host 时设置 `PR_SET_PDEATHSIG`，并负责重启（1/2/4/8/16/30 秒退避加抖动）。正常退出时产品进程向 app-host 发 `shutdown`，app-host 并行发 `app.will_stop`（5000ms），超时 SIGTERM 2s、SIGKILL；systemd `TimeoutStopSec=15`。产品进程被 `kill -9` 或掉电时，PDEATHSIG 立即杀 app-host 与 sidecar，不留孤儿。若目标发行版强制要求 systemd，可换成 systemd unit，但这不是 P0 的前提。

## 2. 数据通道

- **继承的 `socketpair(AF_UNIX, SOCK_SEQPACKET)`**。产品进程先建一对 socket，再 `posix_spawn` app-host，把一端 `dup2` 到固定 fd 传入。app-host 启动后只认这个 fd。
- 没有路径、没有权限位、没有重连。对端身份由内核的 fd 传递保证：别的进程要冒充，必须先拿到这个 fd，而这需要同 uid 的 ptrace 或文件描述符注入，app-host 以独立 uid 运行，产品进程的 fd 不对外暴露。
- 帧格式：`{len, seq, instance_token, opcode, payload}`，单帧 ≤64KB；每连接在途请求有上限与 deadline，超限拒绝。`SOCK_SEQPACKET` 保留消息边界。超时后的迟到回包按 `seq` 丢弃。
- 每实例令牌由产品进程在激活时发放，随帧发送。app-host 能读到所有实例的令牌，因此**A 档是协作式隔离**，不是真正的应用间安全边界；但持有合法令牌的 app-host 仍绕不过产品进程的 grant 检查。
- B 档 sidecar 与 A 档同构：产品进程 `posix_spawn` 后继承一条 `socketpair`，不用文件系统 socket；`SO_PEERCRED` 只留给“不由产品进程拉起”的例外场景。身份主要由 fd 传递与独立 uid 共同保证。
- **sidecar 握手（2026-11-09 已定）**：令牌不走环境变量/命令行。app-host 在继承的 socketpair 上先发 `hello {instance_token, abi, app_id}`，sidecar 回 `hello_ack` 后才收事件；首帧 2 秒超时即杀进程并按启动失败计数。令牌只在内存，不写文件、不打日志；每次重启重新生成，不重用。Go/Python 适配器封装握手，应用拿不到令牌。
- **sidecar 线格式（2026-11-09 已定）**：每帧 u32 长度 + UTF-8 JSON；字段与 WIT 类型一一对应；double 用 JSON number，bytes 用 base64，时间戳用整数毫秒；单帧上限 1MB，超过拒绝并计数。v1 不引入 Protobuf/CBOR；若 p99 RPC 超过预算，再以帧头 `encoding` 协商 CBOR。
- 不用共享内存作为 v1 通道。SHM 延迟更低，但跨进程身份与授权更难做对，且与 DataHub 的零拷贝语义纠缠。等 P1 有实测数据再评估。

## 3. 身份与授权强度

**B 档（sidecar）**：每应用独立 uid；通道与 A 档一样是继承的 `socketpair`，身份由 fd 传递与 uid 共同保证。`SO_PEERCRED` 只用于外部拉起的例外场景。这是真正的应用间安全边界。

**A 档（WASM）**：所有实例共享 app-host 进程。实例令牌由产品进程发放并放在 app-host 内存里，调用时随帧发送。问题很明确：**app-host 能读到所有实例的令牌，所以一个被攻破的 app-host 可以冒充任一实例**。因此：

- A 档能防应用 bug、死循环和越界内存，不能防被攻破的运行时。
- 产品进程仍逐次校验令牌与 grant，能挡住未授权调用；但挡不住持有合法令牌的冒充。
- 需要在文档与能力审批界面明确写"A 档授权强度低于 B 档"。
- 高风险应用（写点位、控制类）只允许 B 档。若 C/C++ 应用需要写能力，必须走 **native sidecar**（B 档）；该档位随 B 档一起实现，不能把写能力留在 A 档。

**授权强制点在产品进程。** app-host 内的检查是纵深防御，不是最终判定。最终判定由产品 ACL 做：解析原生标识、查 grant、比对 `grant_generation`、检查范围、写审计。

**句柄带授权代次。** `point_resolve(name)` 返回 `(handle, grant_generation)`；每次 `point_read(handle)` 由产品进程复查当前代次与范围。撤销时 `grant_generation` +1，已有句柄下一次调用返回 `DENIED_REVOKED`，不要求应用重新 resolve。

## 4. app-host 自身的最小权限

与 B 档同等要求，不能只约束 sidecar：

- 独立 uid/gid，只读根文件系统，应用包目录只读。
- seccomp 白名单，只允许 socket、clock、内存映射与必要的运行时调用。
- 无网络 namespace，除非平台自身需要。
- 不能写注册表，不能读其他应用的数据目录。
- 只持有产品进程传入的那一个 socket fd；不打开文件系统 socket。
- 被产品进程监督，崩溃后重启并重新激活。

## 5. app-manager 与产品进程的分工

- **app-manager 是产品进程内的一个模块**，不是独立进程。它是注册表唯一写者，负责安装、批准、注册表、快照、调谐、状态接口与 `grant_generation`。
- 单写者锁只覆盖 SQLite 短事务；激活与健康检查不在锁内，避免 sidecar 冷启动阻塞撤销。
- **appctl** 经 unix socket 调产品进程；socket 权限 0600，owner 为产品进程 uid。管理身份是本机管理员账号（见 02 号文档），不依赖网络。
- **管理面只监听本地 unix socket** `/run/edgeos/mgmt.sock`，不绑 TCP。`appctl` 直连；WebUI 由 `edge-web`（Go，服务端会话 Cookie）代理到该 socket。授权在 `edge-web` 做一次（用户会话/角色），产品进程再做一次（grant/句柄），两层都过才执行。
- **C 的 `0.0.0.0:9091` REST 默认改为只绑回环或 unix socket**，由 `edge-web` 同机代理；状态变更端点必须带会话断言，错误返回真实 HTTP 状态码，CORS 收敛到同源。直连网络的部署必须显式开启并强制 TLS + 认证，不作为默认。现有 `/api/v1/apps/*` 要么迁到管理 socket，要么加断言，不能保留"无认证上传 WASM 并启动"的路径。
- **Edge-Ops 可选**：只下发设备策略、做车队审批；设备离线时本机批准链路必须完整。Edge-Ops 不能替代本机 `grant` 行。
- 没有独立 supervisor：app-host 的存活由产品进程负责；app-manager 自身随产品进程重启，从数据库重建快照与调谐状态。

## 6. 运行态不进注册表

`restarting`、`faulted`、`isolated`、重启次数属于高频运行态。写进注册表会牵动单写者、快照与 generation。

- 运行态放在产品进程内存与指标里，经状态接口读取（`GET /api/v1/apps/status` 或 appctl）。
- 注册表只存期望状态与已发布状态：`desired_version`、`published_version`、`app.state`。`activating` 由两个指针是否相等推导，不落库。
- 运行态变化不推进 generation，不触发快照重发。
- 只持久化一张很小的"故障连击"表（`app_id`、连续失败次数、最后故障时间，NORMAL 同步，仅在故障跃迁时写），防止整机重启清零绕过隔离阈值。
- 两套阈值（激活失败、运行时崩溃）各自计数，落到同一个 `isolated`；解除条件统一为人工解除或冷却期结束。

### 6.1 崩溃恢复

- **产品进程崩溃**：systemd 拉起（`Restart=on-failure`，`RestartSec=1`）；app-host 因 `PR_SET_PDEATHSIG` 被杀，sidecar 随 app-host 被杀，不留孤儿。启动顺序为：打开控制库 → 重放 WAL → spawn app-host → 按 `desired_state=running` 恢复。
- **app-host 崩溃**：产品进程读到 socketpair EOF，把所有实例标 `restarting`，按 1s/2s/4s/8s/16s/30s 退避重拉；重拉后按依赖拓扑序重新激活。5 分钟内 3 次崩溃则停止重拉，平台进 degraded 并告警，不无限重启。
- **激活顺序**：按 `requires.points` 的应用依赖做拓扑排序；提供者没起来时消费者进 `degraded`，不阻塞其他应用。安装时已拒绝依赖环。
- **手动停止不被复活**：`desired_state=stopped` 后，产品进程重启和 app-host 重拉都不激活该应用；只有管理员显式 `start` 才改回 `running`。

## 7. 数据归属表

| 数据 | 写者 | 读者 | 迁移后 |
| --- | --- | --- | --- |
| 应用清单与版本 | app-manager（产品进程内） | 所有 | `edge_app_mgr` 退化为适配器，不再直接写 |
| `app_registry` | app-manager | 页面、适配器 | 保留为兼容视图 |
| `app_settings` / `app_secret` / `app_kv` | app-manager 的数据服务 | 应用经 ABI | 新表 |
| 应用历史 | 宿主在 `point_publish` 时写 | 查询 API、U1 | 独立应用历史表，键为 `(app_id, point_name, contract_version)` |
| 运行态 | 产品进程内存 | 状态接口 | 不落库（故障连击表除外） |
| grant | app-manager | 产品进程校验 | 新表 |
| 应用点位 name → slot | app-manager | 产品进程 | 注册表贡献；slot 可回收，历史不依赖 slot |
| app-point 运行目录 | 产品进程 | 产品进程（`point_read`/`point_publish`） | 内存：slot → name、owner、`source_ts`、`stale_after_ms`；槽位回收即删，历史不受影响 |
| 注册表快照 | app-manager | UI / 北向 / 验收脚本 | `/run/edgeos/registry/gen-N.json`，四段投影；不含产品点位、grant、参数值；保留两代 |

`edge_app_mgr` 最终是"退化为适配器"还是"被替换"，在迁移文档 M4 阶段决定；在那之前它不能绕过 app-manager 写状态。

## 8. 组件归类

| 单元 | 分类 | 依据 |
| --- | --- | --- |
| app-manager | component（产品进程内模块） | 应用生命周期与注册表，依赖 infra |
| app-host | component | 运行时承载，只经端口访问数据 |
| appctl | 工具 | 管理命令行，走产品进程的管理接口 |
| 产品 ACL | product / 组合根 | 产品端口的实现 |

不再有 supervisor 组件。同时受 `check_component_boundary.py` 与预算门禁约束，`component.meta` 按 key=value 写。

## 9. 验收

1. app-host 由产品进程 `posix_spawn` 拉起，只持有继承的 socket fd；另一个进程无法连上该通道。
2. 帧里 `instance_token` 与实例不匹配，返回拒绝并记审计。
3. 撤销 grant 后，已持有的句柄下一次调用返回 `DENIED_REVOKED`。
4. `kill -9` 产品进程，app-host 因 `PDEATHSIG` 退出，不留下孤儿进程。
5. `kill -9` app-host，产品进程清理该连接的句柄与订阅，按退避重启并重新激活。
6. C/C++ 应用声明 `tags.write` 且 `execution_profile: wasm`，安装被拒；写能力只允许 native sidecar。
7. 运行态变化不推进 generation，不触发快照重发。
8. 状态接口返回重启次数与最后故障码；注册表里没有这些字段。
9. `appctl` 不能绕过产品进程直接写注册表。
10. 组件分类与 `component.meta` 一致，CI 通过。

## 10. 未决

无。A 档令牌轮换与 SHM 已于 2026-11-10 收敛；SHM 只在实测触发条件下评估，见上。
