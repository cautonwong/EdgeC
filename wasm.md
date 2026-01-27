将 Wasm 环境集成到现有的 CMake 构建系统，并确保现有代码的“可控更改”，是工程落地的第一道坎。我们不能因为引入 Wasm 就把现有的 C 代码搞得面目全非。

以下是侵入性最小、自动化程度最高的集成方案：

1. 构建系统集成 (Build System Integration)

我们需要在 CMake 中引入“双轨构建 (Dual-Track Build)”机制：

- Track A (Host): 传统的 GCC/Clang，编译宿主程序（EdgeOS）。
- Track B (Wasm): wasi-sdk 或 emscripten，编译 Wasm 模块。

A. 环境准备
下载 wasi-sdk 并解压到 /opt/wasi-sdk。

B. CMake 脚本增强 (cmake/WasmBuild.cmake)
创建一个专门处理 Wasm 编译的 CMake 模块。

    1 # cmake/WasmBuild.cmake
    2
    3 # 定义 Wasm 编译器路径
    4 set(WASI_SDK_PATH "/opt/wasi-sdk" CACHE PATH "Path to wasi-sdk")
    5 set(WASM_CC "${WASI_SDK_PATH}/bin/clang")
    6 set(WASM_SYSROOT "${WASI_SDK_PATH}/share/wasi-sysroot")
    7
    8 # 定义一个宏，用于将 C 源码编译为 .wasm 并转换为 C 数组
    9 macro(add_wasm_module TARGET_NAME SOURCE_FILE)

10 set(WASM_OUT "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.wasm")
11 set(C_HEADER_OUT "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}\_bytecode.h")
12
13 # 1. 编译 .c -> .wasm
14 add_custom_command(
15 OUTPUT ${WASM_OUT}
   16         COMMAND ${WASM_CC}
   17             --sysroot=${WASM_SYSROOT}
18 -O3 -z stack-size=4096 -Wl,--export=process
19 -Wl,--no-entry -Wl,--allow-undefined
20 -o ${WASM_OUT}
   21             ${SOURCE_FILE}
   22         DEPENDS ${SOURCE_FILE}
   23         COMMENT "Building Wasm Module: ${TARGET_NAME}"
   24     )
   25 
   26     # 2. 转换 .wasm -> .h (十六进制数组)
   27     # 这样主程序可以直接 #include 这个头文件，内置 Wasm 字节码
   28     add_custom_command(
   29         OUTPUT ${C_HEADER_OUT}
   30         COMMAND xxd -i ${WASM_OUT} > ${C_HEADER_OUT}
   31         DEPENDS ${WASM_OUT}
   32         COMMENT "Embedding Wasm Module: ${TARGET_NAME}"
   33     )
   34 
   35     # 3. 创建一个自定义 Target 方便主程序依赖
   36     add_custom_target(${TARGET_NAME}\_gen DEPENDS ${C_HEADER_OUT})
37 endmacro()

C. 在主 CMakeLists.txt 中使用

1 include(cmake/WasmBuild.cmake)
2
3 # 编译内置的示例 Wasm 算法
4 add_wasm_module(algo_demo_wasm "algo/wasm/demo_algo.c")
5
6 # 主程序依赖这个生成过程
7 add_dependencies(edge_stream_engine algo_demo_wasm_gen)

---

2. 代码的可控更改 (Controlled Mutation)

我们不仅要加新功能，还要修改现有代码以支持 Wasm，但必须遵循 “开闭原则”：对扩展开放，对修改关闭。

A. 引入“运行时适配层” (src/stream_engine/wasm_adapter.c)
不要直接修改 stream_engine.c。创建一个全新的适配器文件。

- 职责：封装 WAMR 的初始化、模块加载、函数调用。
- 接口：对外只暴露一个标准的 stage_proc_fn。


    1 // wasm_adapter.c
    2 #include "wasm_export.h" // WAMR 头文件
    3
    4 // 这是 Wasm 算子的通用入口
    5 // 在 YAML 里配置 params.s1 = "内置的 bytecode 数组名" 或 "文件路径"
    6 stage_ret_t stage_wasm_proxy_proc(pipeline_stage_t *s, pipe_msg_t *msg) {
    7     wasm_module_inst_t inst = (wasm_module_inst_t)s->priv_state;
    8
    9     // 1. 准备参数 (将 C 的 pipe_msg_t 映射到 Wasm 内存)

10 // 这里需要处理复杂的内存拷贝，或者直接传值
11 double input = msg->payload.val.f;
12
13 // 2. 调用 Wasm 函数 "process"
14 wasm_function_inst_t func = wasm_runtime_lookup_function(inst, "process", NULL);
15 uint32_t argv[2];
16 // 注意：Wasm 传参通常需要打包
17 _(double_)argv = input;
18
19 if (wasm_runtime_call_wasm(inst, func, 1, argv)) {
20 // 3.以此类推，获取返回值
21 double result = _(double_)argv;
22 msg->payload.val.f = result;
23 return STAGE_OK;
24 }
25 return STAGE_ERROR;
26 }

B. 现有代码的微小改动
你只需要在 stream_engine_register_default_stages 里加一行：

    1 // src/stream_engine/standard_stages.c
    2
    3 void stream_engine_register_defaults(stream_engine_t *engine) {
    4     // ... 原有代码 ...
    5
    6 #ifdef ENABLE_WASM
    7     // 注册 Wasm 代理算子
    8     stream_engine_register_stage_type(engine, "WASM_RUNNER", stage_wasm_proxy_proc);
    9 #endif

10 }

C. 契约层的无感接入
YAML 配置文件完全不需要知道底层发生了什么：

1 stages:
2 - type: "WASM_RUNNER"
3 name: "my_rust_algo"
4 params:
5 s1: "/opt/edgeos/wasm/complex_algo.wasm" # 加载外部文件

3. 如何保证“现有代码不崩”？

1. 特性开关 (Feature Flag)：
   在 CMake 中定义 option(ENABLE_WASM "Enable Wasm Support" OFF)。默认关闭。只有在显式开启时，才会链接 WAMR 库和编译适配层。
1. 沙箱隔离：
   Wasm 运行时的崩溃（Trap）是可捕获的。在 wasm_adapter.c 中，如果不幸发生了 Trap（比如除零），WAMR 会返回错误码，你可以安全地记录日志并跳过该 Stage，绝不会导致主进程 Segfault。
1. 内存池隔离：
   为 WAMR 分配一块独立的 malloc 区域（比如 static char global_heap_buf[512 * 1024]），并通过 wasm_runtime_init 传入。这样即使 Wasm 内部内存泄漏，也不会吃光系统的堆内存。

总结

- 集成方式：wasi-sdk + CMake Custom Command (双轨编译)。
- 代码侵入：极低。新增一个 .c 适配层，修改一行注册代码。
- 发布形态：可以将 .wasm 编译进二进制（内置），也可以运行时加载文件（插件）。

这种方案让你可以在不破坏现有稳定性的前提下，平滑地引入 Wasm 能力。

## 2

在工业场景或生产级 EdgeOS 中，裸奔运行 Wasm 文件是绝对禁止的。你列出的四个安全维度，正是构建“安全沙箱”的基石。

如果只是为了跑个 Demo，可以直接加载；但为了工程落地，我们需要设计一个 Wasm 加载器 (Wasm Loader) 模块，专门负责这些前置检查。

1. 签名校验 (Signature Verification)
   风险：黑客篡改了 /opt/edgeos/wasm/algo.wasm，注入了挖矿代码或逻辑炸弹。
   对策：采用 Ed25519 数字签名。

- 构建时：CI/CD 流水线在编译出 .wasm 后，使用私钥生成签名文件 .wasm.sig。
- 加载时：

1 // wasm_loader.c
2 bool verify_signature(const uint8_t *wasm_buf, size_t len, const uint8_t *sig) {
3 // 使用内置的公钥验证签名
4 return ed25519_verify(sig, wasm_buf, len, g_public_key);
5 }
只有验证通过的二进制流，才允许进入 WAMR 运行时。

2. 版本兼容性校验 (ABI Versioning)
   风险：Wasm 模块是基于旧版 EdgeOS API (v1.0) 编译的，但现在 EdgeOS 升级到了 v2.0，某些 Host Function 签名变了（比如 datahub_read 多了个参数）。直接运行会导致内存错乱或未定义行为。
   对策：在 Wasm 的 Custom Section 中嵌入元数据。

- 构建时：向 .wasm 注入一段 JSON 描述：{"api_version": "1.2.0", "target": "edgeos-aarch64"}。
- 加载时：
  - 解析 Wasm 文件的 Custom Section。
  - 检查 api_version 是否在当前 OS 的兼容列表内（Semantic Versioning 检查）。
  - 如果 OS 是 v2.0，且声明不兼容 v1.x，则拒绝加载。

3. 白名单 Capability (WASI 权限控制)
   风险：一个只需要计算“加法”的 Wasm 模块，却尝试打开 /etc/passwd 或者向外网发送 HTTP 请求。
   对策：最小权限原则 (Least Privilege)。

- 配置：在 YAML 契约中显式声明该 Stage 的权限。

1 stages:
2 - type: "WASM_RUNNER"
3 params:
4 s1: "algo.wasm"
5 # 显式声明权限
6 caps: ["log", "datahub:read:2001"] # 只允许打日志和读 2001

- 运行时拦截：
  在 wasm_adapter.c 导出的 Host Function 中进行鉴权：

1 // Host Function Wrapper
2 void host_file_open(...) {
3 wasm_instance_t \*inst = get_current_instance();
4 if (!has_capability(inst, "fs:read")) {
5 wasm_runtime_set_exception(inst, "Permission Denied: fs:read");
6 return;
7 }
8 // ... 执行打开操作
9 }

4. 资源配额 (Resource Quota)
   风险：死循环、内存泄漏。
   对策：利用 WAMR 的内置机制。

- CPU (Fuel / Metering)：
  - 构建 Wasm 时插入计数指令（Instrumentation）。
  - 运行时设置 fuel_limit = 1,000,000。
  - 每执行一段指令扣除 Fuel，耗尽则 Trap。
- 内存 (Heap Size)：
  - 加载时限制线性内存最大页数（Max Pages）。
  - wasm_runtime_instantiate(..., stack_size=8KB, heap_size=64KB)。
- 时间 (Watchdog)：
  - 虽然有 Fuel，但 Fuel 和挂钟时间不是线性关系（IO 等待不消耗 Fuel）。
  - 在 wasm_proxy_proc 开始时记录时间，结束时检查。或者启动一个辅助定时器，超时未返回则强制销毁 Instance。

总结：Wasm Loader 的工作流

当 StreamEngine 想要加载一个 Wasm Stage 时，真正的流程是：

1.  Read: 读取 .wasm 和 .sig 文件。
2.  Verify: 校验数字签名（防篡改）。
3.  Check: 解析 Custom Section，校验 API 版本（防崩溃）。
4.  Instantiate:
    - 分配受限的内存池。
    - 注入带权限检查的 Host Functions。
    - 设置 CPU Fuel 上限。
5.  Run: 执行 \_start 或 process 函数。

只有加上这四道锁，Wasm 才能真正安全地运行在核电站、变电站这种关键基础设施的边缘网关上

Sprint: "WASM Catalyst" - 集成 WebAssembly 计算引擎

第一阶段：基础设施与构建集成 (Infrastructure)

[Task 1.1] 引入 WAMR 运行时依赖

- 目标：在项目中集成 WASM 执行环境。
- 执行步骤：
  1.  在 .local/ 目录下通过脚本自动下载并编译 iwasm 静态库（或将 WAMR 源码作为 git submodule 引入）。
  2.  确保支持 INTERP（解释执行）和 AOT（预编译执行）模式。
- DoD：运行 ls .local/lib/libvmlib.a 能看到生成的运行时库。

[Task 1.2] CMake 构建系统适配

- 目标：使 EdgeOS 主程序能够链接 WASM 运行时。
- 执行步骤：
  1.  编写 cmake/FindWasm.cmake 模块。
  2.  修改根目录 CMakeLists.txt，增加 ENABLE_WASM 选项。
  3.  将 WASM 运行时链接到 edge_stream_engine 目标。
- DoD：执行 ./build.sh 后，ldd 检查 iems 结果中包含 WASM 相关符号。

---

第二阶段：核心桥接与 ABI 定义 (The Bridge)

[Task 2.1] 定义 EdgeOS-WASM ABI (系统调用)

- 目标：让 WASM 算子能“看”到 DataHub 的数据。
- 执行步骤：
  1.  在 src/stream_engine/wasm_runtime/ 下实现 host_exports.c。
  2.  导出以下 C 函数给 WASM：
      - ext_datahub_read(tag_id): 读取影子表。
      - ext_datahub_publish(tag_id, value): 发布计算结果。
      - ext_log(level, msg): WASM 内部日志打印。
- DoD：代码中完成 NativeSymbol 数组定义，映射 WASM 函数名到 C 函数。

[Task 2.2] 实现零拷贝内存映射 (Zero-Copy Ref)

- 目标：WASM 算子直接处理 10MHz 的 TYPE_REF 原始波形。
- 执行步骤：
  1.  利用 WAMR 的 wasm_runtime_set_native_batch 或共享线性内存特性。
  2.  在调用 WASM 前，将 PipeMsg 携带的 payload.ref.ptr 映射到 WASM 实例的内部地址空间。
- DoD：WASM 脚本通过一个指针直接读取 1000 个浮点数采样点，无需 memcpy。

---

第三阶段：管线集成 (Pipeline Execution)

[Task 3.1] 增加 `wasm_executor` 算子类型

- 目标：在 manifest.yaml 中配置 WASM 任务。
- 执行步骤：
  1.  在 src/stream_engine/ 下创建 stage_wasm.c。
  2.  实现算子初始化：从文件系统加载 .wasm 模块，创建虚拟机实例。
  3.  实现算子执行：将触发 Pipeline 的 PipeMsg 转换为 WASM 函数调用。
- DoD：manifest.yaml 中配置 type: "wasm_executor" 后，IEMS 启动不报错。

---

第四阶段：开发与调试环境 (DevEx)

[Task 4.1] 提供 WASM 算子开发模板 (C/Rust)

- 目标：让第三方开发者知道怎么写算法。
- 执行步骤：
  1.  创建 algo/wasm/template_c/：提供 edgeos.h（WASM 侧头文件）和 Makefile。
  2.  创建 algo/wasm/template_rust/：提供 edgeos-sdk crate。
  3.  提供一键编译脚本：tools/compile_wasm.sh（基于 WASI-SDK）。
- DoD：开发者只需执行 makewasm 即可生成适配 EdgeOS 的 .wasm 文件。

[Task 4.2] 实现 WASM 运行时日志转发

- 目标：在 IEMS 控制台看到 WASM 内部的调试信息。
- 执行步骤：
  1.  重定向 WASM 的 stdout/stderr 到 EdgeOS 的 edge_log 系统。
- DoD：IEMS 日志中出现 [WASM-LOG] Hello from WebAssembly!。

---

第五阶段：发布与热更新 (Lifecycle)

[Task 5.1] 支持算法在线热替换

- 目标：不停机更新 WASM 逻辑。
- 执行步骤：
  1.  扩展 persistence_mgr，监控 algo/ 目录下的 .wasm 文件变更。
  2.  当文件变化时，调用 stage_wasm_reload()：销毁旧实例，加载新模块。
- DoD：在 IEMS 运行时覆盖 my_algo.wasm，新逻辑在 1 秒内生效。

---

🛠️ 推荐工具链建议：

1.  宿主编译器：保持 GCC/Clang。
2.  WASM 编译器：推荐 WASI-SDK（用于 C/C++）或 Rust (wasm32-wasi)。
3.  调试器：利用 WAMR 提供的远程调试协议，支持在 VSCode 中对 WASM 算法打断点。
