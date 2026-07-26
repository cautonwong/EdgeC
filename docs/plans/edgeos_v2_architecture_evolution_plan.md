# EdgeOS V2.0 核心架构演进实施计划与任务分解表

## 📋 项目概览 (Overview)
本计划旨在实现 EdgeOS 三项核心架构演进：
1. **契约动态分块与热迁移 (Dynamic Chunked Schema & Hot Migration)**：支持在线稀疏 mmap 分块与 1 纳秒级 CAS 契约无锁切换，保证点位变更时不重启节点、零物理偏移错位；
2. **DAG 拓扑流计算引擎 (Lock-Free DAG Topology Stream Engine)**：基于原子入度计数器与 Work-Stealing 机制实现多核并行计算与屏障同步（Barrier Sync），并在加载期进行 Tarjan 拓扑无环校验；
3. **分布式 DataHub Mesh 网格 (Distributed DataHub Mesh & State Convergence)**：基于 UDP 组播增量 Delta + QUIC 可靠补包 + Merkle Tree 校验 + MTT 时间戳 CRDT 裁决，实现多节点影子矩阵毫秒级状态收敛。

每个子任务粒度精确控制在 **2~5 分钟** 内，前台负责验收与更新计划状态。

---

## 阶段一：契约动态分块与热迁移 (Phase 1: Dynamic Chunked Schema & Hot Migration)

- [x] **Task 1.1**: 在 `modules/edge_core/include/datahub.h` 中定义 `datahub_chunk_t` 与 `schema_version_t` 结构体，支持稀疏 mmap 段表与原子 Schema 指针
- [x] **Task 1.2**: 在 `modules/edge_core/src/datahub.c` 中实现 `datahub_allocate_chunk()` 稀疏 4KB 页对齐扩展 API
- [x] **Task 1.3**: 在 `datahub.c` 中实现 `datahub_switch_schema()` CAS 无锁契约热切换 API
- [x] **Task 1.4**: 编写并运行 C 单元测试 `tests/test_datahub_dynamic_chunk.c` 验证动态分块与热切换零错位

---

## 阶段二：DAG 拓扑流计算引擎 (Phase 2: Lock-Free DAG Topology Engine)

- [x] **Task 2.1**: 在 `modules/edge_stream/include/stream_engine.h` 中定义 `dag_node_t` 节点与 `dag_graph_t` 拓扑图数据结构
- [x] **Task 2.2**: 在 `modules/edge_stream/src/pipeline_loader.c` 中实现 Tarjan 拓扑强连通分量算法，用于建图期死锁/环路校验 (`dag_validate_acyclic`)
- [x] **Task 2.3**: 在 `modules/edge_stream/src/stream_engine.c` 中实现基于 CAS 原子计数器 (`atomic_fetch_sub`) 的多核 Work-Pool 并发调度与屏障同步
- [x] **Task 2.4**: 编写并运行 C 单元测试 `tests/test_stream_dag.c` 验证 DAG 多核并行计算与拓扑防环

---

## 阶段三：分布式 DataHub Mesh 网格 (Phase 3: Distributed DataHub Mesh)

- [x] **Task 3.1**: 在 `modules/edge_core/include/edge_mesh.h` 中定义 `mesh_delta_pkt_t` 增量数据包结构与 `merkle_tree_node_t` 哈希树切片
- [x] **Task 3.2**: 在 `modules/edge_core/src/edge_mesh.c` 中实现基于 UDP 组播的 Delta 增量广播与 MTT 挂钟时间戳 CRDT 冲突裁决 API
- [x] **Task 3.3**: 在 `edge_mesh.c` 中实现 Merkle Tree 根哈希计算与增量补包恢复机制 (`mesh_sync_reconcile`)
- [x] **Task 3.4**: 编写并运行 C 单元测试 `tests/test_edge_mesh.c` 验证多节点影子矩阵毫秒级收敛与丢包恢复

---

## 阶段四：全量集成与系统级端到端验证 (Phase 4: Full System Verification)

- [x] **Task 4.1**: 运行 `./build.sh` 确保所有架构修改与新增 C 模块无编译警告及链接错误
- [x] **Task 4.2**: 运行 `./test.sh` 确保全量 C 单元测试套件 100% 通过
