## 1. pipelines.json 在咱们这个项目中  如何加载的 如何执行的  整个生命周期

这是 **EdgeOS SDK** 中“业务逻辑定义”的核心。

`pipelines.json` 不仅仅是一个配置文件，它实际上是 **“低代码逻辑编排”** 的脚本。在 C 语言环境中，我们不能像 Java/Python 那样利用反射动态加载类，因此我们需要设计一套 **“注册表 + 解释器”** 机制来加载和执行它。

以下是 `pipelines.json` 从磁盘上的文本到内存中高效运行的 C 代码的全生命周期详解。

---

### 1. 文件的物理形态 (The Definition)

首先，回顾一下它长什么样。它定义了数据流经过的一系列 **工序 (Stages)**。

**`config/pipelines.json`**
```json
{
  "pipelines": [
    {
      "id": "voltage_monitor",
      "enable": true,
      // 触发条件：所有电压类型的 Tag
      "triggers": [ { "type": "DATAHUB", "source": "meter/voltage/#" } ],
      "stages": [
        // 工序 1: 线性变换 (y = kx + b)
        { "name": "TransformScale", "config": { "k": 0.1, "b": 0 } },
        // 工序 2: 告警检查 (阈值判断)
        { "name": "AlarmCheck", "config": { "max": 250.0 } },
        // 工序 3: 数据上云
        { "name": "ExportMQTT", "config": { "topic": "data/upload" } }
      ]
    }
  ]
}
```

---

### 2. 生命周期全景图 (Lifecycle Overview)

整个流程跨越了 **L1 (Main Loop)** 和 **L2 (Stream Loop)**。

```text
[ Disk ] pipelines.json
    |
    v (1. Load & Parse)
[ L1 Main Loop ]
    | cJSON 对象
    v (2. Compile / Link)
[ Function Registry ] (查表: String Name -> Function Pointer)
    |
    v (3. Build Object Graph)
[ Pipeline Objects ] (链表结构: Stage A -> Stage B -> Stage C)
    |
    v (4. Atomic Swap)
[ L2 Stream Loop ] <--- 拥有 Pipeline 链表的执行权
    |
    | (5. Trigger & Exec)
    v
[ DataHub Event ] --> [ Execute Stages ]
```

---

### 3. 详细步骤拆解

#### 阶段一：启动与注册 (Boot & Registry)
**执行位置**：`src/core/pipeline/pipeline_mgr.c` (L1)

在加载 JSON 之前，C 代码必须先建立一个 **“函数注册表”**。这是 C 语言实现动态逻辑的基石。

```c
// 定义函数指针类型
typedef int (*stage_func_t)(pipe_ctx_t *ctx, void *config, PipeMsg *msg);

// 全局注册表 (Hash Map or Array)
structFuncEntry { char *name; stage_func_t func; } g_registry[] = {
    { "TransformScale", stage_transform_scale },
    { "AlarmCheck",     stage_alarm_check },
    { "ExportMQTT",     stage_export_mqtt },
    { NULL, NULL }
};

// 系统启动时调用
void pipeline_register_all() {
    // 注册所有内置函数
    // 也可以通过 dlopen 加载 .so 里的函数并注册
}
```

#### 阶段二：加载与编译 (Load & Compile)
**执行位置**：L1 Main Loop

这是将 JSON 文本转换为 C 结构体的过程。

1.  **读取**：`uv_fs_read` 读取文件内容。
2.  **解析**：`cJSON_Parse` 转为 JSON 对象。
3.  **链接 (Linking)**：
    *   遍历 JSON 中的 `stages` 数组。
    *   拿到 `"name": "TransformScale"`。
    *   在 `g_registry` 中查找对应的函数指针 `stage_transform_scale`。
    *   `malloc` 一个 `edge_stage_t` 结构体，填入函数指针和配置参数。
4.  **组装**：将 Stage 结构体串成链表，挂在 `edge_pipeline_t` 对象上。

```c
// 内存中的 Pipeline 对象
typedef struct edge_stage_s {
    stage_func_t func;      // 已链接的 C 函数 (Fast!)
    cJSON *config;          // 该阶段的私有配置
    struct edge_stage_s *next;
} edge_stage_t;
```

#### 阶段三：分发与激活 (Dispatch & Activate)
**执行位置**：L1 -> L2 (跨线程)

Pipeline 对象在 L1 构建好后，不能直接用，必须**安全地移交**给 L2 Stream Engine。

1.  L1 发送 `CMD_UPDATE_PIPELINES` 信号给 L2 的 `uv_async_t`。
2.  L2 收到信号，在当次 Loop 的空闲间隙，更新自己的 `pipeline_list` 指针。
    *   *注意*：如果是**热更新**，L2 会先释放旧的 Pipeline 链表，再挂载新的。

#### 阶段四：运行时执行 (Runtime Execution)
**执行位置**：L2 Stream Engine Loop

这是 **高频热路径 (Hot Path)**。

1.  **触发 (Trigger)**：
    *   DataHub 收到数据，触发 `on_datahub_notify`。
    *   L2 遍历 `pipeline_list`。
    *   **匹配器**：检查 `pipeline->triggers` 是否匹配当前数据的 `TagID` 或 `Topic`。
2.  **执行 (Run)**：
    *   如果匹配，进入 `pipeline_run(pipe, msg)`。
    *   **循环调用函数指针**：
        ```c
        // 极速循环，无字符串匹配，无 Hash 查找
        while (stage) {
            int ret = stage->func(ctx, stage->config, msg);
            if (ret == PIPE_DROP) break; // 丢弃
            if (ret == PIPE_ASYNC) break; // 转异步
            stage = stage->next;
        }
        ```

---

### 4. 关键问题：配置参数如何传递给 C 函数？

在 JSON 中，`TransformScale` 有参数 `{"k": 0.1}`。C 函数怎么拿到？

**方案 A：每次解析 (慢，灵活)**
Stage 函数直接操作 `cJSON *config` 对象。
```c
int stage_transform_scale(..., void *config_obj, ...) {
    cJSON *cfg = (cJSON*)config_obj;
    double k = cJSON_GetObjectItem(cfg, "k")->valuedouble; // 每次查找，慢！
    msg->val *= k;
}
```

**方案 B：预处理上下文 (快，推荐)**
在 **阶段二 (Compile)** 时，增加一个 `init` 步骤，让 Stage 把 JSON 转为 C 结构体。

```c
// 注册时提供 init 函数
typedef void* (*stage_init_t)(cJSON *config);

// 编译阶段调用
void *priv_data = ops->init(json_config); // 解析 JSON，存入 struct scale_cfg { double k; };
stage->priv_data = priv_data;

// 运行时调用
int stage_transform_scale(..., void *priv_data, ...) {
    scale_cfg_t *cfg = (scale_cfg_t*)priv_data;
    msg->val *= cfg->k; // 直接内存访问，极速！
}
```

---

### 5. 高级特性：热重载 (Hot Reload)

当用户修改了 `pipelines.json`：
1.  **L1** 监听到文件变化 (inotify)。
2.  **L1** 重新执行 **阶段二 (Compile)**，构建出一棵**新的 Pipeline 树**。
    *   *此时旧的 Pipeline 还在 L2 中跑着，互不影响。*
3.  **L1** 发送指针给 L2。
4.  **L2** 在处理完当前这一批数据后，执行 **指针交换 (Swap)**。
    *   `current_pipelines = new_pipelines;`
5.  **L2** 把旧的指针发回给 L1（或者放入延迟释放队列）。
6.  **L1** 销毁旧对象。

**结果**：用户修改配置，系统**不重启、不丢包**，逻辑平滑切换。

### 总结

`pipelines.json` 的加载执行机制，本质上是一个 **“静态编译的虚拟机”**。

1.  **JSON** 是源代码。
2.  **L1** 是编译器，把它变成了函数指针链表 (Bytecode)。
3.  **L2** 是 CPU，全速执行这些函数指针。

这种设计既保留了 JSON 配置的灵活性，又确保了运行时 C 语言级别的执行效率（消除了运行时字符串匹配和 JSON 解析）。


## 2. 目录/workspaces/edgeos/EdgeC/tools/contract-compiler/templates是空的? /workspaces/edgeos/EdgeC/tools/contract-compiler/protocol目录下只有三个文件? 这符合预期吗?

在工业级契约治理中，覆盖顺序必须严格遵守“越往现场，权力越大”的原则。目前的 Go 编译器在
  merger.go 中先加载了 Local（现场扩展），随后又去 append 了
  Includes（库文件），导致现场定义的物理属性被库文件里的默认值给堆叠甚至“淹没”了。

  真正的契约覆盖定力（优先级从低到高）：

   1. Layer 1: 全局字典 (`tags.yaml`) —— 基准定义（名称、ID、单位）。
   2. Layer 2: 协议库 (`bindings/*.yaml`) —— 默认映射（设备模版的寄存器地址、缩放系数）。
   3. Layer 3: 本地扩展 (`extension.yaml`) ——
      现场真相（现场实际接线的地址、根据传感器量程修正的系数）。
   4. Layer 4: 产品微调 (`products/*.yaml`) —— 最终裁决（对特定产品进行的最后 Overrides）。

  我现在的 merger.go 逻辑是先 3 后 2，这完全搞反了。

  修正方案：

   1. 重整合并顺序：先解析 Includes（库），再加载 Local（现场）。
   2. 从 Append 改为 Upsert：在处理 Binding 时，如果同一个协议（如
      MODBUS）已经有了映射，现场定义必须直接覆盖库定义，而不是简单的 append。
   3. 物理闭环：确证 PROJECT_HUMIDITY 的 scale: 1.0 在最终的 contracts.c 中战胜库里的 0.1。

   接下来，我建议我们将重心转向 Stream Engine 的“业务算法库” 建设，或者继续深化 IO Executor
  的异步并发性能优化。您有何指示？