# 11 进程拓扑与可信计算基

状态：设计基线，待评审。未实现。

## 1. 拓扑

| 进程 | 职责 | 与谁通信 | 身份与授权 |
| --- | --- | --- | --- |
| 网关主进程 | 产品 ACL、DataHub 影子值、北向、采集。**授权最终强制点** | app-host、sidecar | 校验通道身份、实例令牌与 `grant_generation` |
| app-manager | 唯一写者：安装、批准、注册表、快照、调谐；发放实例令牌 | 主进程、supervisor、消费者心跳 | 管理员身份；写 grant 前校验权限 |
| supervisor | 进程存活、重启、崩溃归因 | app-manager、app-host、sidecar | 只能启停，不读业务数据 |
| app-host | 承载 N 个 WASM 实例 | 主进程（数据通道）、app-manager | 独立 uid、seccomp、只读根；每实例令牌 |
| sidecar | 承载 Go/Python 应用 | 主进程（每应用独立 socket） | 每应用 uid，`SO_PEERCRED` 认证 |

网关主进程不加载应用代码。应用代码只在 app-host 与 sidecar 里。

## 2. 数据通道

`point_read` 必须跨进程，因为影子值与产品 ACL 在主进程。v1 的通道选择：

- **unix socket，长度前缀帧**，每帧带 `app_id`、`instance_token`、`profile`、`native_id`、操作码与载荷。
- 一个 app-host 一条连接，多实例复用，靠 `instance_token` 区分。sidecar 每应用一条独立连接。
- 不用共享内存作为 v1 通道。SHM 延迟更低，但跨进程的身份与授权更难做对，且与 DataHub 的零拷贝语义纠缠。等 P1 有实测数据再评估。
- 单帧上限 64KB，单连接在途请求上限固定，超限拒绝。

**为什么不用 DataHub 直连。** ADR-0011 规定组件不得 include `datahub.h`，只能通过端口。app-host 是组件，直连会破坏边界，也让授权绕过产品 ACL。

## 3. 身份与授权强度

A 档与 B 档的授权强度不同，必须写明。

**B 档（sidecar）**：每应用独立 uid，socket 用 `SO_PEERCRED` 取对端 uid，按 uid 映射到应用。身份由内核提供，应用无法伪造。这是真正的应用间安全边界。

**A 档（WASM）**：所有实例共享 app-host 进程。实例令牌由 app-manager 发放并放在 app-host 内存里，调用时随帧发送。问题很明确：**app-host 能读到所有实例的令牌，所以一个被攻破的 app-host 可以冒充任一实例**。因此：

- A 档是**协作式隔离**，能防应用 bug、死循环和越界内存，不能防被攻破的运行时。
- 主进程仍然逐次校验令牌与 grant，能挡住未授权调用；但挡不住持有合法令牌的冒充。
- 需要在文档与能力审批界面明确写"A 档授权强度低于 B 档"。
- 高风险应用（写点位、控制类）只允许 B 档。A 档只用于只读点位与计算类。

**授权强制点在主进程。** app-host 内的检查是纵深防御，不是最终判定。最终判定由产品 ACL 做：解析原生标识、查 grant、比对 `grant_generation`、检查范围、写审计。

## 4. app-host 自身的最小权限

与 B 档同等要求，不能只约束 sidecar：

- 独立 uid/gid，只读根文件系统，应用包目录只读。
- seccomp 白名单，只允许 socket、clock、内存映射与必要的运行时调用。
- 无网络 namespace，除非平台自身需要。
- 不能写注册表，不能读其他应用的数据目录。
- 被 supervisor 监督，崩溃后重启并重新激活。

## 5. supervisor 与 app-manager 的分工

- app-manager 决定**期望状态**（该跑什么），写注册表与快照。
- supervisor 只负责**观测与拉起**（进程活没活），不决定版本，不读业务数据。
- supervisor 崩溃归因：共享内存里维护"在途实例"面包屑，记录最近一次调用属于哪个实例；崩溃后先逐个重新激活，定位毒应用，再全量恢复。
- 崩溃恢复走事件触发，不等 30 秒调谐周期。调谐周期只是兜底。

## 6. 运行态不进注册表

`restarting`、`faulted`、`isolated`、重启次数属于高频运行态。写进注册表会牵动单写者、快照与 generation。

- 运行态放在 app-manager 内存与指标里，页面经状态接口读取。
- 注册表只存期望状态与已发布状态：`desired_version`、`published_version`、`app.state`。
- 运行态变化不推进 generation，不触发快照重发。

## 7. 数据归属表

| 数据 | 写者 | 读者 | 迁移后 |
| --- | --- | --- | --- |
| 应用清单与版本 | app-manager | 所有 | `edge_app_mgr` 退化为适配器，不再直接写 |
| `app_registry` | app-manager | 页面、适配器 | 保留为兼容视图 |
| `app_settings` / `app_secret` / `app_kv` | app-manager 的数据服务 | 应用经 ABI | 新表 |
| 运行态 | app-manager 内存 | 页面 | 不落库 |
| grant | app-manager | 主进程校验 | 新表 |

`edge_app_mgr` 最终是"退化为适配器"还是"被替换"，在迁移文档 M4 阶段决定；在那之前它不能绕过 app-manager 写状态。

## 8. 组件归类

`app-manager`、`app-host`、`supervisor`、`appctl` 需要补进 `COMPONENT_CLASSIFICATION`：

| 单元 | 分类 | 依据 |
| --- | --- | --- |
| `app-manager` | component（支持子域） | 应用生命周期与注册表，依赖 infra |
| `app-host` | component | 运行时承载，只经端口访问数据 |
| `supervisor` | infra | 进程监督，不认识业务 |
| `appctl` | 工具 | 管理命令行，走管理接口 |
| 产品 ACL | product / 组合根 | 产品端口的实现 |

同时受 `check_component_boundary.py` 与预算门禁约束，`component.meta` 按 key=value 写。

## 9. 验收

1. app-host 以独立 uid 运行，无法写注册表与产品数据目录。
2. sidecar 连接用 `SO_PEERCRED` 认证，伪造 uid 的连接被拒。
3. A 档实例令牌与 app_id 绑定，跨实例复用令牌被拒。
4. 主进程对每次调用校验 grant 与 `grant_generation`，撤销后下一次调用被拒。
5. 直接 include `datahub.h` 的 app-host 实现被边界门禁拦下。
6. supervisor 崩溃后，用面包屑定位到具体实例，只重启该实例。
7. 运行态变化不推进 generation，不触发快照重发。
8. `appctl` 不能绕过 app-manager 直接写注册表。
9. 组件分类与 `component.meta` 一致，CI 通过。

## 10. 未决

- A 档实例令牌的轮换周期。
- SHM 通道是否在 P1 引入，取决于 p99 实测。
- supervisor 用 systemd 还是自研，取决于目标发行版。
