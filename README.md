# EdgeOS (EdgeC) - 工业级边缘计算核心运行时 SDK

EdgeOS 是一个专为 **工业物联网 (IIoT)**、**能源管理系统 (EMS)** 和 **智能电网融合终端** 打造的高性能、确定性、事件驱动型边缘计算 SDK。它采用 C11 标准构建，核心基于 `libuv` 事件循环，旨在 15 年以上的超长生命周期内提供稳如磐石的服务。

---

## 🏗️ 1. 核心架构设计 (The "Six Roles" Philosophy)

EdgeOS 将复杂的边缘计算逻辑解耦为六大核心角色，每个角色在物理或逻辑上相互隔离，确保系统在极端负载下依然具备可观测性和可恢复性。

### 1.1 系统角色分布图
| 角色 | 关键组件 | 技术栈 | 职责描述 |
| :--- | :--- | :--- | :--- |
| **Arbiter** | `arbiter.c` | Priority Queue | **总线主权者**。管理物理总线（RS485/CAN）的优先级调度、抢占式访问及 OTA 租约锁定。 |
| **DataHub** | `shmring.c` | Atomic SHM | **系统事实源**。基于共享内存的无锁环形缓冲区，实现纳秒级数据分发与跨线程/进程通知。 |
| **IO Executor** | `io_executor.c` | Thread Pool | **物理执行器**。每个总线拥有独立线程，处理阻塞式 IO，支持 Mock 模式进行无硬件仿真。 |
| **Logic Engine** | `js_engine.c` | QuickJS | **算法沙箱**。执行 C 原生算法或动态下发的 JavaScript 脚本，具备 2MB 内存配额强制限制。 |
| **Persistence** | `sqlite_writer.c` | SQLite WAL | **记忆中枢**。单写者模式异步存储历史遥测、日冻结数据及系统配置，确保主线程零阻塞。 |
| **Northbound** | `rest_server.c` | libuv TCP | **交互门户**。分离的静态 HMI (:8080) 与 REST API (:8081)，支持全场景数据流（S1-S9）展示。 |

---

## 🧠 2. 动态规则引擎 (JavaScript Sandbox)

EdgeOS 集成了 **QuickJS** 作为其动态逻辑层，使系统从“静态采集器”进化为“可编程智能终端”。

*   **沙箱安全性**：
    *   **内存配额**：强制限制每个脚本实例占用内存（默认 2MB），防止脚本错误导致系统 OOM。
    *   **时间限制**：内置执行中断钩子，防止死循环脚本挂起控制主循环。
*   **EdgeOS SDK for JS**：
    *   `DataHub.read(tag_id)`：零拷贝读取实时共享内存数据。
    *   `Arbiter.control(tag_id, value)`：安全地向南向总线发送高优先级控制指令。
    *   `Log.info(msg)`：直接向系统日志流输出结构化信息。
*   **热加载加载器 (Hot Reloading)**：
    *   实时监控 `config/scripts/` 目录。
    *   无需重启即可下发新的报警策略、数据清洗算法或控制逻辑。

---

## 🌐 3. 北向交互体系 (Northbound Stack)

EdgeOS 采用了物理隔离的北向双服务器架构，确保人机交互与机器通信互不干扰。

### 3.1 RESTful JSON API (:8081)
专为上级网关或 App 设计的机器接口：
*   `GET /api/v1/live`：毫秒级返回全量点位快照。
*   `GET /api/v1/points`：获取当前系统加载的物理点位配置。
*   `POST /api/v1/control/write`：执行反向控制（S6 场景）。
*   `GET /events`：基于 **SSE (Server-Sent Events)** 的高频数据推送流。

### 3.2 Professional Commander UI (:8080)
内置工业级大屏 HMI，基于 **TailwindCSS + AlpineJS + Chart.js** 构建：
*   **实时趋势图**：高采样率展示电压、功率等动态曲线。
*   **状态监视器**：玻璃拟态设计的核心指标卡片，支持呼吸灯状态显示。
*   **控制台**：直观的滑块与开关，直接操作南向设备。
*   **动静分离**：UI 资源位于 `config/www/`，支持前端独立快速迭代。

---

## 🚀 4. 快速上手

### 4.1 编译项目
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

### 4.2 运行产品
```bash
./iems -m iems -w 8080 -r 8081 --config ../config/device_profile.json
```

### 4.3 动态逻辑示例
在 `config/scripts/` 下创建一个 `alarm.js`：
```javascript
var v = DataHub.read(1001);
if (v > 245) {
    Log.info("Voltage too high: " + v + "V!");
    Arbiter.control(2001, 0); // 触发保护
}
```

---

## 🛠️ 5. SDK 开发规范

*   **Zero Globals**：SDK 核心严禁使用 `static` 或全局变量，所有状态必须通过上下文句柄传递。
*   **Thread Safety**：跨线程通信必须通过 `uv_async_send` 或 `shm_ring`。
*   **Memory Discipline**：优先使用 `net_buf` 处理报文，确保引用计数正确归零。
*   **Code Style**：项目强制使用 `clang-format`。推荐安装 `pre-commit` 钩子以实现提交时自动格式化：
    ```bash
    pip install pre-commit
    pre-commit install
    ```

---

EdgeOS 为工业边缘侧带来了 **Web 级的灵活性** 与 **内核级的稳定性**。