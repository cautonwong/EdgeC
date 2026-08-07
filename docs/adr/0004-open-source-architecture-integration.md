# ADR-0004: 顶级开源技术架构集成与演进规范 (Open Source Architecture Integration)

> **Status**: Accepted (已批准)  
> **Date**: 2026-08-07  
> **Authors**: Mr. WKT, Agentic AI Architect  
> **Deciders**: Mr. WKT (The Boss)  

---

## 1. 上下文与背景 (Context & Background)

EdgeOS / EdgeC 作为工业级边缘操作系统，已具备物理影子总线（DataHub）、确定性调度（MTT）、契约驱动开发（CDD）以及双轨通信等核心能力。

为了将系统的吞吐性能、跨语言交互效率、工业协议标准兼容性以及多设备级联能力提升至国际一流水平，我们需要汲取并融合 GitHub 顶级开源项目的优秀架构设计。

---

## 2. 四大开源项目集成方案 (The Four Open Source Pillars)

### 2.1 open62541 (C99 原生工业级 OPC UA 引擎)
* **借鉴对象**：[open62541/open62541](https://github.com/open62541/open62541)
* **应用场景**：`modules/edge_io/src/engines/opcua_engine.c` 及工业主站对接。
* **架构方案**：
  1. 将 open62541 作为 C99 纯标准库写入依赖层（`.local/` 与 CMake 联合构建）。
  2. 将 OPC UA VariableNode 的指针直接绑定至 DataHub `edge_lookup_tag()` 得到的物理内存偏移量，消除二次拷贝。
  3. 保持与现有 `Kconfig` 的降级隔离支持（`CONFIG_ENABLE_OPCUA`）。

### 2.2 Cap'n Proto / FlatBuffers (C/Go 内存布局显式对齐)
* **借鉴对象**：[Cap'n Proto](https://github.com/capnproto/capnproto) & [Google FlatBuffers](https://github.com/google/flatbuffers)
* **应用场景**：DataHub 共享内存 (`shmring`) 的 C 语言内存块与 Go 语言访问接口。
* **架构方案**：
  1. 在 `contract-compiler-v2` 的后端生成器中，使用 `#pragma pack(push, 1)` 与显式字节填充（Padding），确保跨编译器/架构的内存布局完全一致。
  2. Go 侧使用 `unsafe.Pointer` 直接映射 `shmring` 的 `mmap` 区域，实现 **0ns 延迟、0 内存分配 (0 B/op)** 的解引用访问。
  3. 增设编译期与运行期的结构体大小静态断言 (`static_assert` / Go `unsafe.Sizeof`)。

### 2.3 eBPF Ring Buffer & BPF Arena (Linux 内核极速事件采集)
* **借鉴对象**：[Cilium](https://github.com/cilium/cilium) & [Falco](https://github.com/falcosecurity/falco)
* **应用场景**：高频网络包采样、ADC/串口硬件中断的高频数据采集。
* **架构方案**：
  1. 在 `modules/edge_io/src/ebpf/` 下提供内核级 `bpf_ringbuf` 采样程序。
  2. 采用共享内存映射机制，数据从内核态直接注入 DataHub 物理矩阵，避免频繁 Syscall 上下文切换。
  3. 在内核版本低于 5.8 或 Zephyr 嵌入式环境下，平滑降级至 `libuv` 异步 IO 机制。

### 2.4 Eclipse Zenoh (分布式多节点 DataHub Mesh)
* **借鉴对象**：[Eclipse Zenoh](https://github.com/eclipse-zenoh/zenoh)
* **应用场景**：SmartBox、IEMS、DCU 等多台边缘设备之间的去中心化影子总线级联。
* **架构方案**：
  1. 在 `modules/edge_mesh/` 中基于 Zenoh-C 封装 Topic 空间：`edgeos/{product_id}/{tag_id}`。
  2. 当 DataHub 发生 `push: "change"` 阈值事件时，通过 Zenoh 进行局域网零拷贝组播。
  3. 自动 P2P 设备发现，无需依赖集中式云端 Broker。

---

## 3. 落地演进路线 (Implementation Roadmap)

| 阶段 | 模块 | 关键交付物 | 验收标志 |
|---|---|---|---|
| **Phase 1** | `open62541` 集成 | OPC UA 真实 Server/Client 引擎 | UaExpert 可直接读写 DataHub 标签 |
| **Phase 2** | C/Go 内存对齐 | 契约编译器与 Go 侧 `unsafe` 指针访问 | Go 侧读取 SHM 实现 0 B/op 内存分配 |
| **Phase 3** | eBPF 采集 | `bpf_ringbuf` 采样与平滑降级 | 上下文切换降低 80%，高频采样无丢包 |
| **Phase 4** | Zenoh Mesh | 多设备物理影子总线级联 | Omnis 多节点物理仿真秒级对齐 |

---

## 4. 后果与评估 (Consequences)

* **正面影响**：
  - 系统兼具极高的单机性能（C/Go 指针解引用 + eBPF）与工业兼容性（OPC UA）。
  - 为多设备边缘协作（SmartGrid / Microgrid）打下了分布式架构基础。
* **负面影响/风险**：
  - eBPF 依赖特定 Linux 内核版本，需确保降级分支测试充分。
  - C/Go 的指针级解引用需要极其严格的内存对齐测试，防止 Panic。

---
