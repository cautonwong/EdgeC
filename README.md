# EdgeOS (EdgeC) - 工业级边缘计算核心运行时 SDK

EdgeOS 是一个专为 **工业物联网 (IIoT)**、**能源管理系统 (EMS)** 和 **智能电网融合终端** 打造的高性能、确定性、事件驱动型边缘计算 SDK。它采用 C11 标准构建，核心基于 `libuv` 事件循环，旨在 15 年以上的超长生命周期内提供稳如磐石的服务。

---

## 🏗️ 1. 核心架构设计 (The "Six Roles" Philosophy)

EdgeOS 将复杂的边缘计算逻辑解耦为六大核心角色，每个角色在物理或逻辑上相互隔离，确保系统在极端负载下依然具备可观测性和可恢复性。

### 1.1 系统角色分布图
| 角色 | 关键组件 | 技术栈 | 职责描述 |
| :--- | :--- | :--- | :--- |
| **Arbiter** | `arbiter.c` | Priority Queue | **总线主权者**。管理物理总线（RS485/CAN）的优先级调度、抢占式访问及 S7 OTA 租约锁定。 |
| **DataHub** | `shmring.c` | Seqlock SHM | **系统事实源**。基于 Seqlock 的无锁环形缓冲区，支持数据全生命周期追踪（TraceState）。 |
| **Scheduler** | `sched_*.c` | Calendar Time | **时间权威**。抗时钟跳变的日历调度，确保分钟曲线、小时冻结和周期抄表任务在校时回拨下不重跑。 |
| **Logic Engine** | `script_*.c` | QuickJS | **隔离沙箱**。在专用线程中执行 JS 算法，具备内存上限强制限制与熔断（Circuit Breaker）保护逻辑。 |
| **Persistence** | `sqlite_svc.c` | SQLite WAL | **存储中心**。支持 DCU 模式下的按月自动分表（Sharding）与 Schema 自动迁移框架。 |
| **Northbound** | `rest_server.c` | Native libuv | **交互门户**。纯 libuv 实现的 HTTP/WebSocket 网关，支持 10Hz 级高频遥测广播与 RESTful 管理接口。 |

---

## 🧠 2. 确定性调度与时间 (Scheduling & Resilience)

不同于普通的 `cron`，EdgeOS 的调度系统专为电力与能源计量场景设计：
*   **抗跳变日历时间 (Calendar Authority)**：利用 Wall Clock 定义日历边界，Monotonic Time 驱动执行。即使系统时间被 NTP 强行校准或人为回拨，已完成的冻结任务绝不重跑。
*   **对齐执行**：支持 15/30/60 分钟对齐，确保曲线数据的合法性。
*   **幂等保证**：每个日历周期通过持久化状态位确保“一生仅执行一次”。

---

## 🛡️ 3. 隔离式逻辑沙箱 (JavaScript sandbox v1.1)

EdgeOS 将脚本执行从主循环中剥离，确立了严格的执行边界：
*   **单执行器线程 (Executor Thread)**：所有 JS 逻辑在独立计算线程中串行执行，杜绝脚本死循环卡死系统 IO。
*   **资源配额与熔断**：
    *   **内存/时间双限**：512KB 堆上限 + 10ms 执行时间预算。
    *   **冷冻状态机**：若脚本连续失败 5 次，系统将进入 **FROZEN** 模式并开启冷却期，随后通过 **PROBE** 探测恢复。
*   **可追踪性 (Traceability)**：每一条 `PipeMsg` 均携带 `TraceState`（CREATED -> SHM -> STORED -> EXPORTED），运维人员可通过 UI 观察数据流向。

---

## 🌐 4. 纯净北向网关 (Native Web Gateway)

EdgeOS 摒弃了沉重的 Web 库，仅依赖 `libuv` 原生套接字：
*   **极致轻量**：手搓 WebSocket 协议栈（SHA1/Base64 算法内置），无 OpenSSL 等外部库强依赖。
*   **可观测性 HMI**：
    *   **Topology 视图**：实时监控 Arbiter 调度、总线忙闲及任务排队深度。
    *   **Points 视图**：动态查阅测点快照。
    *   **Audit 视图**：记录关键控制动作审计日志。

---

## 🚀 5. 快速上手

### 5.1 编译项目
```bash
./build_and_run.sh  # 自动化构建、静态检查与单元测试
```

### 5.2 运行 IEMS 示例
```bash
./build/iems -m iems -w 8080 -r 8081
```
*   访问 `http://localhost:8080` 进入 HMI 控制台。
*   API 端点：`ws://localhost:8081/ws`。

---

## 🛠️ 6. 开发规范

*   **Hermetic Build**：依赖库预编译于 `.local/`，严禁格式化工具（clang-format）触碰此目录。
*   **Zero Globals**：核心严禁使用全局变量，确保多实例共存。
*   **Atomic Interaction**：跨线程状态交换强制使用 Seqlock 保护。
*   **Commit Hooks**：
    ```bash
    pre-commit install  # 安装自动 clang-format 与 YAML 校验钩子
    ```

---

EdgeOS 为工业边缘侧带来了 **Web 级的灵活性** 与 **内核级的稳定性**。
