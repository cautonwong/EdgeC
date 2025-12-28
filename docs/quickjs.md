## QuickJS 作为生产级沙箱 执行(速度要求不特别高的)算法? 方案如何

将 QuickJS（或其轻量化变体）作为 EdgeOS
  的算法沙箱是一个非常成熟且符合工业趋势的选择。相比于目前文档中提到的 TinyExpr，QuickJS
  提供了真正的脚本能力，能够处理逻辑分支、状态保持和复杂的数据结构。

  以下是针对 EdgeOS 架构的 QuickJS 生产级沙箱方案建议：

  1. 定位与角色 (The Scripting Engine Role)
  在 EdgeOS 的“六大角色”中，QuickJS 引擎应属于 Logic & Strategy Engine (Algorithm)。
   * 输入：从 DataHub (shm_ring) 订阅实时数据。
   * 处理：执行用户编写的 .js 脚本（逻辑、控制算法、告警过滤）。
   * 输出：将计算结果写回 DataHub 或通过 Arbiter 下发指令。

  2. 核心方案设计

  A. 沙箱隔离 (Hard Sandboxing)
  QuickJS 的优势在于其内置的资源限制能力，这对于工业边缘侧的稳定性至关重要：
   * 内存限制：使用 JS_SetMemoryLimit(rt, limit)。为每个脚本分配独立的
     JSContext，限制其内存占用（如 2MB），防止脚本错误导致系统 OOM。
   * 时间/指令限制：通过 JS_SetInterruptHandler(rt, cb)
     设置“看门狗”。如果脚本执行超过预定指令数（Gas
     Limit）或时间，强制中断，防止脚本死循环挂起 Main Loop。
   * 能力裁剪：在初始化时，不要 加载 os 和 std 模块。只注入 EdgeOS 提供的安全 API（如
     DataHub.read, Log.info）。

  B. 数据交互 (Zero-Copy Binding)
  为了让 JS 访问 DataHub 中的 PipeMsg：
   * C-to-JS Bridge：利用 QuickJS 的
     JS_NewExternalArrayBuffer。将共享内存中的数据块直接映射为 JS 的 ArrayBuffer 或使用
     JS_SetPropertyFunctionList 绑定原生 C 函数。
   * 示例 API 设计：

   1     // 脚本示例
   2     const voltage = DataHub.getValue(1001); // 映射到 shm_ring 查找
   3     if (voltage > 240) {
   4         Arbiter.requestControl(2001, { status: 0 }); // 下发控制
   5     }

  C. 线程模型 (Execution Model)
  由于 QuickJS 是一个纯解释器（且非常快），你有两种选择：
   1. Main Loop 模式：如果脚本非常简单（类似规则引擎），直接在 libuv 的回调中执行。依靠
      InterruptHandler 保证不阻塞。
   2. Worker Thread 模式（推荐）：为 QuickJS 开启一个独立的 uv_thread。
       * 脚本在后台运行，通过 uv_async_send 与主线程通信。
       * 这样即便脚本崩溃，也不会影响核心的 I/O 采集。

  3. 为什么选择 QuickJS 而不是 TinyExpr 或 Lua？

  ┌──────────┬──────────────────┬────────────┬────────────────────────────────────────────┐
  │ 特性     │ TinyExpr         │ Lua        │ QuickJS                                    │
  ├──────────┼──────────────────┼────────────┼────────────────────────────────────────────┤
  │ 功能     │ 仅数学公式       │ 完整脚本   │ 现代 JS (ES2023)                           │
  │ 生态     │ 无               │ 工业界常用 │ 极佳（前端开发者秒上手）                   │
  │ 安全性   │ 极高（功能受限） │ 需手动加固 │ 原生支持内存/时间限制                      │
  │ 内存占用 │ 极小 (KB级)      │ 小 (MB级)  │ 中 (约 1-2MB 运行时)                       │
  │ 异步支持 │ 无               │ 协程       │ 原生支持 Promise/Async (对 libuv 极其友好) │
  └──────────┴──────────────────┴────────────┴────────────────────────────────────────────┘

  4. 实施建议 (Implementation Path)

   1. 引入依赖：
      将 QuickJS 加入 vendor/。由于 setup_third_party.sh
  目前遇到网络问题，建议将其源码直接作为 Submodule 或直接复制一份经过精简的
  quickjs-libc.c/h。
   2. 封装 Service：
      创建 src/services/js_engine.c，封装 js_engine_init 和 js_engine_run_script。
   3. 双向绑定：
       * C -> JS：把 shm_ring 的数据包装成 JSObject。
       * JS -> C：把 sqlite_exec_async 和 arbiter_request 暴露给 JS。

  总结
  QuickJS 是生产级沙箱的理想选择。
  它不仅解决了“速度要求不特别高”的算法需求，更重要的是它为 EdgeOS
  带来了动态化能力——你可以通过云端下发一段 JS
  代码，在不重启系统的情况下更新边缘端的控制逻辑，这在 IEMS 和 SG-EL2025
  场景中非常有竞争力。

在我们的 EdgeOS 项目中引入 QuickJS，可以极大地提升系统的动态灵活性。由于项目底层是高性能 C
  语言，很多逻辑（如 IEMS 的削峰填谷、SG-EL2025 的告警逻辑）如果写死在 C
  代码里，每次修改都要重新编译固件，维护成本极高。

  以下是 QuickJS 在我们项目中的几个核心用法方案：

  1. 动态规则引擎 (Dynamic Rule Engine)
  这是最直接的用法。用户可以通过云端下发 JS 脚本，在边缘端实现复杂的逻辑判断。
   * 用法示例：

    1     // 云端下发的规则脚本: rule_alarm.js
    2     const voltage = DataHub.read(1001); // 读取 DataHub 中的电压值
    3     const current = DataHub.read(1002);
    4
    5     if (voltage > 245 && current > 50) {
    6         // 触发告警事件，写入 DataHub
    7         DataHub.write(9001, "OVERVOLTAGE_ALARM", Severity.CRITICAL);
    8         // 通过 Arbiter 下发紧急跳闸指令
    9         Arbiter.requestControl(5001, { action: "TRIP", delay: 0 });
   10     }
   * 工程实现：在 Algorithm 模块中，为每个规则创建一个 QuickJS 运行时环境，并在 DataHub
     数据更新时触发执行。

  2. 协议插件与数据清洗 (Data Transformation)
  南向驱动采上来的原始寄存器数据通常需要转换（缩放、位运算、单位转换）。
   * 用法示例：
      在 Driver Manager 的配置文件中，不再只写 scale: 0.1，而是支持一段 JS 代码：

   1     // 处理复杂的非线性传感器
   2     function on_raw_data(val) {
   3         // 假设是一个非线性的温度传感器转换公式
   4         return (val * 0.534) + (Math.log(val) * 0.2);
   5     }
   * 工程实现：驱动层在解析完 Modbus/CAN 报文后，调用 QuickJS 函数进行转换，再存入 shm_ring。

  3. 虚拟点位计算 (Virtual Points / Soft Sensors)
  实现“计算点”。比如：总功率 = 相A功率 + 相B功率 + 相C功率。
   * 用法示例：

   1     // 聚合计算脚本
   2     const pA = DataHub.read("PhaseA_Power");
   3     const pB = DataHub.read("PhaseB_Power");
   4     const pC = DataHub.read("PhaseC_Power");
   5
   6     DataHub.publish("Total_Power", pA + pB + pC);
   * 工程实现：利用 libuv 的定时器，每 100ms 运行一次 JS 聚合脚本。

  4. 边缘侧北向数据映射 (Northbound Payload Mapping)
  不同的云平台要求不同的 JSON 格式（阿里云、华为云、私有平台）。
   * 用法示例：

   1     // 将 DataHub 结构映射为云端 JSON
   2     function map_to_cloud(data_points) {
   3         return {
   4             deviceId: "EDGE_001",
   5             timestamp: Date.now(),
   6             payload: data_points.filter(p => p.quality === 'GOOD')
   7                                 .map(p => ({ k: p.tag, v: p.val }))
   8         };
   9     }
   * 工程实现：在 Northbound Mgr 发送前，调用 JS 脚本进行格式化。

  5. 联动策略管理 (Logic & Strategy Engine)
  处理 IEMS 场景中的复杂状态机。
   * 用法示例：

   1     // 削峰填谷策略状态机
   2     switch(System.getMode()) {
   3         case Modes.PEAK:
   4             if (Battery.soc > 20) Battery.discharge(Target.MAX);
   5             break;
   6         case Modes.VALLEY:
   7             Battery.charge(Target.ECO);
   8             break;
   9     }

  ---

  集成到 EdgeOS 的关键技术细节：

  1. 资源配额管理 (Resource Quota)
  这是工业级沙箱的必做项：
   * 内存限制：调用 JS_SetMemoryLimit。例如限制每个脚本只能用 2MB，防止 JS 里的 while(true) {
     list.push(...) } 撑爆系统。
   * 执行时间限制：利用 JS_SetInterruptHandler。如果脚本执行超过 50ms
     还没返回，直接强制杀死，保证 Main Loop 的实时性。

  2. C 语言绑定 (C-JS Bridge)
  你需要把我们现有的 C 接口暴露给 JS：
   * js_edge_datahub_read(tag_id) -> 对应 C 层的 shm_ring_read。
   * js_edge_arbiter_request(...) -> 对应 C 层的 arbiter_request。

  3. 线程模型建议
   * 不要在主线程（Main Loop）里直接跑复杂的 JS。
   * 建议在 io_executor 之外，增加一个 logic_executor 线程池。JS 任务在线程池里跑，跑完后通过
     uv_async_send 把结果回传给主线程。

  总结建议：
  在你的 Phase 4 (Service Plane) 中，可以增加一个 `ScriptingService`。
   * 底层：QuickJS。
   * 接口：提供标准的 edge_js_load(file) 和 edge_js_eval(code)。
   * 价值：有了 QuickJS，EdgeOS 就从一个“固定的采集器”变成了一个“可编程的边缘智能终端”，这对于
     SG-EL2025 这种复杂的电力融合终端是极具竞争力的特性。

  如果你决定开始集成，我可以帮你写一个 `edge_js_binding.c` 的模板，展示如何把 `DataHub`
  的数据实时传给 QuickJS。
