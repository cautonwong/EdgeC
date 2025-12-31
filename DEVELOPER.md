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

### 4.2 调试技巧与宏规范

EdgeOS 采用分级调试机制，严禁直接使用原始 `printf`。



#### C 侧调试标准

*   **LOG_DEBUG**: 仅在 `VERBOSE_LOGGING` 开启时输出，用于追踪总线报文细节。

*   **LOG_INFO**: 用于记录状态迁移（如 Job Start/Complete）。

*   **LOG_ERROR/FATAL**: 用于记录资源故障，FATAL 会触发同步落盘并 `abort()`。



#### JS 侧调试标准

在算法脚本中，请遵循以下模式：

```javascript

export function on_event(event, state) {

    if (params.debug_mode) {

        console.log(`[DEBUG] Received Tag: ${event.tag} Val: ${event.val}`);

    }

}

```



---



## 🌐 5. Web UI 差异化开发规范



### 5.1 路由枚举 (ROUTER_PATH)

前端 Shell (`web/core`) 采用统一的路由定义，禁止在子产品中分叉：

```typescript

enum ROUTER_PATH {

  HOME = '/',

  DASHBOARD = '/dashboard',

  DEVICES = '/devices',

  DATA_CURVES = '/data/curves',

  PIPELINES = '/pipelines',

  SYSTEM_CONFIG = '/system/config'

}

```



### 5.2 差异化策略：Descriptor 驱动

差异化开发遵循以下铁律：

1.  **UI Shell 永远不分叉**：基础布局、Rest API 封装必须在 `web/core` 中完成。

2.  **差异写在 YAML，不写在 JS**：通过后端下发的 `ui_config` 控制页面显隐。

3.  **UI 只渲染，不决策**：前端禁止包含 `if (product === 'iems')` 逻辑，应查询 `contractStore.isPageVisible()`。



---



## 🛡️ 6. 质量红线与测试



### 6.1 应用分类 (Application Types)

*   **EVENT_DRIVEN**: 南向驱动与流引擎 Pipeline。

*   **BACKEND**: REST Server 与持久化管理。

*   **FULLSTACK**: 包含嵌入式 Web 门户的完整产品。



### 6.2 单元测试模式

所有核心功能必须通过 `tests/` 下的 cmocka 验证。

*   **Mocking**: 优先使用 `-Wl,--wrap` 或 `engine_time_source_t` 指针注入。

*   **Assertion**: 验证逻辑结果（如 `-1 != 0`）并确证资源 100% 回收。
