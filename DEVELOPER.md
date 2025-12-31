# EdgeOS Developer Guide (v2.0 Architecture)

欢迎进入 EdgeOS 工业级二代开发体系。本指南旨在指导开发者如何高效地在 EdgeOS 平台上构建高性能、可观测、软硬解耦的边缘应用。

---

## 🏗️ 1. 架构定力：四层逻辑平面

EdgeOS 将复杂的逻辑解耦为四个物理隔离的平面，开发者应根据职责将代码归位：

| 目录 | 平面名称 | 职责描述 | 约束 |
| :--- | :--- | :--- | :--- |
| `src/core` | Foundation (L1) | 总线 (DataHub)、参数中心、内存池。 | **ABI 级稳定**，禁止包含具体产品头文件。 |
| `src/main_loop` | Control Plane | 仲裁器 (Arbiter)、调度器、REST/Web。 | 处理系统级决策，不直接执行高频算法。 |
| `src/stream_engine`| Compute Plane | 这里的每一个 Pipeline 都是一个独立 FSM。 | **禁阻塞 IO**，耗时计算需通过 STAGE_ASYNC。 |
| `src/io_executor` | Execution Plane | 协议驱动、设备采集。 | 必须保持“契约盲”，仅调用查表 API。 |

---

## 📜 2. 数据宪法：契约驱动开发 (CDD)

不要在代码中硬编码任何寄存器地址或测点名称。遵循以下流程：

### 2.1 新增测点流程
1.  **定义语义**：在 `contracts/schema/tags.yaml` 中注册测点（ID、名称、UI 量程）。
2.  **绑定物理层**：在 `contracts/bindings/` 下的对应协议 YAML 中建立映射（如 `0x0034 -> GRID_FREQ`）。
3.  **产品启用**：在 `contracts/products/<prod>.yaml` 中将该测点加入 `enabled_tags`。
4.  **生成与编译**：运行构建，编译器会自动产生 `TAG_XXX` 枚举和 `edge_lookup_xxx` 查表函数。

### 2.2 驱动层使用示例
```c
#include "contracts.h" // 包含当前产品的专属宪法
...
float scale;
edge_tag_id_t id = edge_lookup_modbus(0x0100, &scale);
if (id != TAG_INVALID) {
    // 自动感知测点含义并应用量纲缩放
}
```

---

## 🚀 3. 构建与产物

EdgeOS 采用多产品沙箱构建模型。

### 3.1 编译指令
```bash
./build_and_run.sh  # 全量编译并运行测试
```

### 3.2 产物分布
*   **SDK 静态库**：`build/sdk/lib/` (libedge_core.a 等，一次编译全场共享)。
*   **产品二进制**：`build/products/<name>/bin/` (独立沙箱，不冲突)。
*   **审计契约**：`build/products/<name>/generated/contracts.yaml` (前端与运维的单一事实源)。

---

## 🔍 4. 日志治理与调试

EdgeOS 实现了基于事件驱动的异步日志系统，日志文件统一输出在项目根目录的 **`logs/`** 文件夹中。

### 4.1 日志分类 (Category)
请根据模块职责选择正确的分类，以便进行针对性的文件分流：

| 宏定义 | 物理文件 | 场景 |
| :--- | :--- | :--- |
| `LOG_CAT_CORE` | `core.log` | 系统启动、容器管理、配置变更。 |
| `LOG_CAT_BUS` | `bus.log` | DataHub 读写、共享内存竞态排查。 |
| `LOG_CAT_SCHED` | `sched.log` | 仲裁器抢占、任务流转。 |
| `LOG_CAT_DRIVER` | `driver.log` | Modbus/DLMS 原始报文与映射逻辑。 |
| `LOG_CAT_STORAGE` | `storage.log` | SQLite 写入性能、数据一致性。 |

### 4.2 调试技巧
*   **实时查看**：`tail -f logs/driver.log | grep "TAG_GRID_POWER"`
*   **丢包监控**：定期调用 `edge_log_drop_count()`。如果该值增长，说明日志 IO 负载过高或缓冲区溢出。
*   **FATAL 处理**：`LOG_FATAL` 会执行同步落盘并立即调用 `abort()`，产生的 core 文件是定位崩溃的第一现场。

---

## 🛡️ 5. 质量红线



1.  **Libuv 优先**：禁止直接使用 `pthread` 或 `poll`，必须封装为 `uv_thread` 或 `uv_poll`。

2.  **异步闭环**：所有 `STAGE_ASYNC` 必须匹配对应的 `pipeline_resume`。

3.  **内存定力**：严禁在 `Stream Loop` 中使用 `malloc`，请优先使用 `NetBuf` 内存池。



---



## 🏗️ 6. 二次开发标准动作 (Secondary Development)



EdgeOS 鼓励通过“增加插件”而非“修改内核”的方式进行扩展。



### 6.1 开发一个新产品 (Creating a Product)

假设你要开发一个名为 `my_gateway` 的产品：

1.  **建立目录**：`mkdir -p cmd/my_gateway`。

2.  **定义契约**：

    *   在 `contracts/products/my_gateway.yaml` 中定义标准测点。

    *   在 `cmd/my_gateway/extension.yaml` 中定义本地私有测点。

3.  **编写入口**：参考 `cmd/iems/main.c` 编写 `main.c`，并注入 `service_set_contract_path`。

4.  **注册构建**：在 `cmd/my_gateway/CMakeLists.txt` 中调用 `edge_add_contract`。



### 6.2 开发算法算子 (Algo Development)



#### 6.2.1 Native C 算子 (核心/高频)

适用于高频采样（>100Hz）或需要极致性能的场景（如 FFT）。

*   **编写代码**：在 `src/stream_engine/` 下新建源文件，实现 `stage_proc_fn` 接口。

*   **状态管理**：使用 `stage->priv_state` 存储跨采样点的上下文。

*   **注册**：在 `pipeline_loader.c` 的 `build_stage` 中增加映射。



```c

// 示例：简单限值告警算子

stage_ret_t my_alarm_proc(pipeline_stage_t *stage, pipe_event_t *ev) {

    double limit = *(double*)stage->priv_cfg;

    if (ev->payload.val > limit) {

        LOG_WARN(LOG_CAT_SCHED, "Threshold exceeded: %.2f", ev->payload.val);

    }

    return STAGE_OK;

}

```



#### 6.2.2 JS 策略脚本 (业务/易变)

适用于需要快速迭代、不希望重启进程的业务逻辑。

*   **编写脚本**：在 `algo/js/` 目录下创建 `.js` 文件。

*   **接口规范**：必须导出 `on_event(event, state)` 函数。

*   **配置生效**：在 Pipeline JSON 配置中使用 `type: "js_rule"`。



```javascript

// 示例：削峰填谷策略

export function on_event(event, state) {

    if (event.tag === 1003 && event.val > 5000) {

        state.overload_count = (state.overload_count || 0) + 1;

        if (state.overload_count > 5) {

            // 触发控制动作

        }

    }

}

```



### 6.3 算法调试与验证



1.  **模拟数据注入**：在 `main.c` 中开启 `dummy_producer` 定时器，向 `datahub` 持续推送测试数据。

2.  **实时日志观测**：

    *   驱动层：`tail -f logs/driver.log`

    *   计算层：`tail -f logs/core.log` (聚合与算子输出)

3.  **单元测试 (推荐)**：

    *   在 `tests/` 下为你的 Algo 编写 cmocka 测试。

    *   利用 `pipe_event_t` 的 Mock 注入，验证算法在“时光回溯”或“极速暴增”下的稳定性。
