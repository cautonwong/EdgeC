# AEP-452 纪律与架构红线自检清单 (Redlines)

| 红线检查项 | 约束标准 | 达标情况 | 证明依据 / 备注 |
|---|---|:---:|---|
| **Boss Protocol** | 每轮以 Mr. WKT 开头 | ✅ PASS | 全程强制遵循上下文焦点对齐 |
| **DDS 节点自发现与失活检测** | 动态追踪节点加入、租约超时与离线重连自愈 | ✅ PASS | 实现 `dds_topology_monitor.c/.h` |
| **DataHub 关联点位自动失效自愈** | 节点离线时自动将其负责点位打标为 QUALITY_INVALID (0x02) | ✅ PASS | 写入 DataHub 影子总线 |
| **Partition QoS 隔离** | 支持电气间隔 Partition 匹配与通配符隔离 | ✅ PASS | 实现 `dds_topology_monitor_match_partition` |
| **物理真实性 (Physical Truth)** | 无 Mock 占位符、真实 DataHub 读写与时钟差值计算 | ✅ PASS | 基于真实 DataHub 影子总线与毫秒级时钟 |
| **手术级修改 (Surgical Edit)** | 仅改动 DDS 基础设施与单测配置 | ✅ PASS | 新增 `modules/edge_infra/bus_dds/dds_topology_monitor.c` 等 |
| **反熵核心只读门禁** | `STRICT_CORE_GATE=1 check_core_readonly.sh` | ✅ PASS | 0 违规核心修改，核心只读门禁通过 |
| **反熵符号基线门禁** | `STRICT_CORE_GATE=1 check_core_symbols.sh` | ✅ PASS | `libedge_core.a` 0 外部禁止符号 |
| **二进制尺寸门禁** | `STRICT_SIZE_GATE=1 check_binary_sizes.sh` | ✅ PASS | 4/4 产品二进制严控尺寸预算通过 |
| **生成物可信链门禁** | `STRICT_DRIFT_GATE=1 check_generated_drift.sh` | ✅ PASS | 编译器一致(delta:0B)，全生成物零漂移 |
| **独立分支与证据包** | ticket/AEP-452, summary/tests/redlines/diffstat | ✅ PASS | 独立分支 `ticket/AEP-452`，全量 `tests.log` 实测入库 |
