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

## 🧠 3. 逻辑核心：流水线 (Pipeline)

Pipeline 是 EdgeOS 流处理引擎 (`Stream Engine`) 的最小逻辑单元。它采用 **事件驱动 + 有状态计算** 的模式，负责将南向采集的“生数据”转化为具备业务价值的“事实”。

 1. algo/ 目录目前的定位：业务算法资产库
  在 v2.0 架构中，src/stream_engine/standard_stages.c 承载了 SDK
  核心算子（如基础过滤、量纲转换、万能累加）。
  而 `algo/` 目录 现在转变为 业务/行业算法仓：
   * `algo/native/`：存放特定行业的 C 算法实现（如：储能独有的
     battery_strategy.c、电力品质分析等）。它们不属于 SDK 内核，但作为插件供各产品消费。
   * `algo/js/`：存放高频变动的业务策略脚本。这些脚本由 js_rule 算子在运行时加载，实现“逻辑热更新”。
   * `algo/include/`：暴露业务算法的接口，供 pipeline_loader 或 main.c 引用。

  2. DEVELOPER.md 补全：对齐最新的分类与路由规范
  根据您提供的代码片段，我们需要在 DEVELOPER.md
  中补全应用分类（ApplicationType）、日志等级（LogLevel）以及前端路由（ROUTER_PATH）的标准化定义。这将确保
  所有二开产品的行为逻辑在“元数据”层面保持一致。

  3. 二开添加算法的标准工作流 (Workflow)
  我为您梳理了“算法进 Pipeline”的工业级 SOP：

  路径 A：添加 Native C 算法（高性能/行业级）
   1. 定义：在 algo/include/ 定义数据结构，在 algo/native/ 实现逻辑（遵循 stage_proc_fn 契约）。
   2. 契约：若涉及新测点，在 contracts/schema/tags.yaml 注册。
   3. 注册：在 src/stream_engine/pipeline_loader.c 的 build_stage 注册表里增加 type 映射。
   4. 配置：在产品的 pipelines.json 中配置该 type。

  路径 B：添加 JS 策略算法（业务级/动态）
   1. 编写：在 algo/js/ 下编写 .js 文件，导出 on_event(event, state)。
   2. 配置：无需修改 C 代码，直接在 JSON 中定义 type: "js_rule" 并指定 script_path 即可立即生效。


### 3.1 Pipeline 的工作原理
*   **触发机制**：基于 `TagID` 的 Topic 订阅。当 DataHub 产生匹配的数据点时，关联的 Pipeline 被激活。
*   **顺序执行**：一个 Pipeline 包含多个 `Stage`（环节）。数据按顺序流经各个 Stage，每个 Stage 都可以修改数据或决定是否截断流转。
*   **非阻塞定力**：所有 Pipeline 运行在独立的 `Stream Loop` 线程中。若某个 Stage 需要执行耗时操作（如写数据库），它会返回 `STAGE_ASYNC` 挂起当前 Pipeline，待完成后恢复，绝不阻塞总线。

### 3.2 编写 Pipeline (JSON DSL)
流水线通过配置文件（如 `config/iems_pipelines.json`）进行定义，无需修改 C 代码即可重构业务逻辑：

```json
{
  "pipelines": [
    {
      "id": "battery_safety_chain",
      "trigger_tag": 4001, 
      "stages": [
        { "type": "filter_range", "config": { "min": 0, "max": 100 } },
        { "type": "battery_guard", "config": { "max_charge_w": 5000 } },
        { "type": "js_rule", "config": { "script_path": "algo/js/charge_policy.js" } },
        { "type": "sqlite_sink", "config": { "table": "battery_logs" } }
      ]
    }
  ]
}
```

### 3.3 开发自定义 Stage (C 语言)
如果你需要实现一个行业特有的算子，必须遵循以下契约：

#### A. 实现处理函数
```c
stage_ret_t my_custom_proc(pipeline_stage_t *stage, pipe_event_t *ev) {
    if (ev->type != EVT_DATA_POINT) return STAGE_OK;

    // 访问私有状态 (跨采样点保留)
    MyState *s = (MyState*)stage->priv_state;
    
    // 执行算法逻辑
    if (ev->payload.val > 100.0) {
        return STAGE_DROP; // 截断：后续 Stage 不再接收此事件
    }
    
    return STAGE_OK; // 继续流向下一个 Stage
}
```

#### B. 算子状态机返回值 (stage_ret_t)
| 返回值 | 引擎行为描述 |
| :--- | :--- |
| `STAGE_OK` | 当前环节完成，数据流向下游。 |
| `STAGE_DROP` | 立即停止当前流水线，丢弃该事件。常用于 Filter 或 异常拦截。 |
| `STAGE_ASYNC` | **核心特性**。挂起流水线，释放 CPU。直到调用 `pipeline_resume`。 |
| `STAGE_ERROR` | 发生故障，流水线进入错误统计计数。 |

---

## 🚀 4. 构建与产物

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







## 🛡️ 6. 质量红线与分类规范







### 6.1 应用分类 (ApplicationType)



所有部署在 EdgeOS 上的应用必须归属于以下分类之一，以便系统进行资源调度优化：



```python



class ApplicationType(str, Enum):



    EVENT_DRIVEN = 'event_driven' # 响应总线数据的算法 (Stream Engine)



    BACKEND      = 'backend'      # 提供 REST/管理能力的服务



    FULLSTACK    = 'fullstack'    # 带 Web HMI 的完整产品



```







### 6.2 日志分级规范 (LogLevel)



严禁使用自定义级别，必须对齐以下枚举：



```python



class LogLevel(Enum):



    DEBUG    = 'debug'



    INFO     = 'info'



    WARNING  = 'warning'



    ERROR    = 'error'



    CRITICAL = 'critical' # 对应 C 侧的 LOG_FATAL



```







---







## 🌐 7. Web UI 导航契约 (Routing)







前端 Shell (`web/core`) 采用硬性路由枚举，子产品通过 Descriptor 控制可见性，严禁修改路径名称：



```typescript



enum ROUTER_PATH {



  HOME = '/',



  DASHBOARD = '/dashboard',



  TRAIN = '/dashboard/train',



  PLAYGROUND = '/dashboard/playground',



  APPLICATIONS = '/dashboard/applications',



  STANDALONE = '/standalone'



}



```







---







## 🏗️ 8. 二次开发标准动作 (Secondary Development)







### 8.1 开发一个新产品 (Creating a Product)



1.  **建立目录**：`mkdir -p cmd/my_gateway`。



2.  **定义契约**：在 `contracts/products/` 定义 YAML，并在 `cmd/my_gateway/extension.yaml` 注入私有测点。



3.  **编写入口**：在 `main.c` 中注入 `service_set_contract_path`。



4.  **注册构建**：在产品子目录 `CMakeLists.txt` 中调用 `edge_add_contract`。







### 8.2 添加算法算子工作流 (Algorithm Workflow)







#### A. Native C 算子（高性能/高频）



*   **实现**：在 `algo/native/` 实现 `stage_proc_fn` 接口。



*   **契约**：在 `tags.yaml` 注册 TagID。



*   **注册**：在 `pipeline_loader.c` 的 `build_stage` 注册表中增加 `type`。



*   **部署**：在产品的 `pipelines.json` 中挂载。







#### B. JS 策略脚本（业务级/动态）



*   **编写**：在 `algo/js/` 编写 `.js`，导出 `on_event(event, state)`。



*   **部署**：在 JSON 中定义 `type: "js_rule"` 并指定 `script_path`。




