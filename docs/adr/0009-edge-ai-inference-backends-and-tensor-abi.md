# ADR-0009: 工业边缘 AI 推理引擎分轨选型与 WASM Tensor ABI 架构宪法

## 状态 (Status)
**Accepted** (2026-10-18)

## 关联票据 (Related Tickets)
- **Issue #218**: [arch] 边缘AI：清理“名义 TinyML”并确定推理后端（TinyMaix/TFLite-Micro vs ONNX/TVM），先出 ADR
- **Issue #219**: [arch] 边缘AI：WASM 缺 tensor ABI 与模型契约 —— 窗口采了即丢，真模型无处挂载
- **Issue #223 (AEP-604)**: 工业边缘轻量化量化算子融合 (Operator Fusion) 与无锁双缓冲模型热插拔引擎
- **Issue #229 (AEP-610)**: 异构边缘 NPU HAL 与 DMA-BUF 零拷贝硬件加速

---

## 背景与问题陈述 (Context & Problem Statement)

在之前的系统审计中，发现了边缘 AI 面存在的两个严重架构与物理真实性脱节问题：
1. **名义 TinyML 与伪实现漂移 (Issue #218)**：部分自称“TinyML”的算子（如 `radar_tinyml_operator.c`）实为硬编码阈值 `if (vel > 1.8 && micro_amp > 0.4)`，无任何真实权重矩阵和张量计算，形成虚假的交付闭环；
2. **WASM Tensor ABI 缺失与窗口数据采了即丢 (Issue #219)**：`wasm_mgr.c` 仅提供 `host_get_tag` / `host_set_tag` 单标量读写接口。`stage_tinyml_wasm.c` 收集满滑动窗口样本后，由于缺少张量 ABI，在代码注释中坦陈“此处简化”，实际上并未将窗口推入 WASM 内存，任何真实的多维神经网络模型均无法挂载运行。

---

## 核心架构决策 (Architectural Decisions)

### 决策 1：按产品 Tier 严格分轨的推理后端选型 (Tier-Separated AI Engine)

| 产品 Tier | 目标硬件平台 | 推理引擎选型 | 运行形态 | 核心理由 |
| :--- | :--- | :--- | :--- | :--- |
| **Basic Tier** | STM32 / Cortex-M4 / RTOS | **C 原生 / TinyMaix** | 静态编译直出 (AOT/Native) | 无 MMU、RAM < 512KB，彻底排除重型 WASM 运行时，杜绝不必要的内存开销。 |
| **Standard Tier** | Cortex-A7/A53 / Linux | **WAMR + Tensor ABI** | 安全沙箱动态 App | 需要热插拔微应用隔离，时序特征提取与浅层 ML 借助标准 Tensor ABI 注入。 |
| **Pro / Enterprise** | RK3588 / Ascend / NPU | **原生 NPU HAL (AEP-610) + DMA-BUF** | Linux 原生进程/零拷贝直连 | 重型视觉/大模型严禁通过 WASM 解释执行，直接走 DMA-BUF 零拷贝直连硬件 NPU。 |

### 决策 2：WASM 核心运行时标准 Tensor ABI 规范 (Issue #219)

在 `wasm_mgr` 中落盘工业级张量内存交互标准 ABI，赋予 Host 与 WASM Guest 双向受控张量流转能力：

```c
// 1. 张量内存管理 (Caller / Host Allocated inside WASM Module Heap)
int wasm_app_tensor_alloc(wasm_app_t *app, uint32_t size_bytes, uint32_t *out_wasm_ptr);
int wasm_app_tensor_free(wasm_app_t *app, uint32_t wasm_ptr);

// 2. 双向零拷贝/批量内存同步
int wasm_app_tensor_write(wasm_app_t *app, uint32_t wasm_ptr, const void *src, uint32_t size_bytes);
int wasm_app_tensor_read(wasm_app_t *app, uint32_t wasm_ptr, void *dst, uint32_t size_bytes);

// 3. 多参模型推理执行器
int wasm_app_exec_args(wasm_app_t *app, const char *func_name, uint32_t argc, uint32_t argv[]);
```

### 决策 3：端到端滑动窗口与流计算装配闭环 (`stage_tinyml_wasm.c`)

彻底重写 `stage_tinyml_wasm.c`：
1. 流管道在初始化时在 WASM 堆中预分配 `in_tensor` 与 `out_tensor`；
2. 当时序点位填满滑动窗口 `ctx->window` 后，通过 `wasm_app_tensor_write` 将全部真实物理时序灌入 WASM 张量空间；
3. 调用模型推理入口函数 `ctx->entry_func(in_ptr, in_count, out_ptr, out_count)`；
4. 通过 `wasm_app_tensor_read` 读取模型分类与置信度向量，完成分类后写回 DataHub 影子寄存器。

### 决策 4：正名与去伪存真 (Issue #218)

1. 将纯规则/阈值逻辑的文件明确正名为“启发式规则算子 (Heuristic Rule Operator)”，代码中明确声明无神经网络模型，杜绝误导；
2. 建立真实的矩阵乘加 (MatMul) 微神经网络端到端集成测试，以确定性数学输出（$\mathbf{y} = \mathbf{W}\mathbf{x} + \mathbf{b}$）作为推理验证基线。

---

## 交付与验收基准 (DoD & Verification)

1. **失败测试先行**：提供 `tests/test_wasm_tensor_abi.c` 验证张量内存分配、多参数传递及矩阵乘推导；
2. **消除代码伪注释**：彻底删除 `stage_tinyml_wasm.c` 中“窗口未推入”的简化说明，代之以真实 Tensor ABI 写入与读取；
3. **架构防腐**：算法公共头文件不污染核心层，Tier 边界清晰；
4. 全量 `./build.sh`、`./test.sh` 与 Omnis 仿真 100% 变绿通过。
