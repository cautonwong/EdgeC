# ARCH-DEEP-03 纪律与架构红线自检清单 (Redlines)

| 红线检查项 | 约束标准 | 达标情况 | 证明依据 / 备注 |
|---|---|:---:|---|
| **Boss Protocol** | 每轮以 Mr. WKT 开头 | ✅ PASS | 全程强制遵循上下文焦点对齐 |
| **统一时序仓储抽象** | 单一深度 Seam 封装所有持久化细节 | ✅ PASS | 实现 `edge_telemetry_store_append`, `edge_telemetry_store_query`, `edge_telemetry_store_flush_sync` |
| **WAL 崩溃一致性保证** | 异常断电/强杀重启后数据 100% 自动恢复且 0 损坏 | ✅ PASS | 实现二进制 `WALT` 预写日志与 CRC32 完整性校验，单测实测 50/50 记录零损恢复 |
| **透明 Gorilla 时序压缩** | 测点数据自动进入时序压缩引擎 | ✅ PASS | 自动采集 `ts_sample_t` 并批处理压缩，透明统计 `compression_bytes_in/out` |
| **无缝断线重发 (SnF)** | 离线暂存并在恢复后平滑回放 | ✅ PASS | 实现 `edge_telemetry_store_set_online`，实测离线 100 条暂存并在连线时完整回灌 |
| **物理真实性 (Physical Truth)** | 无 Mock 占位符、真实二进制文件 IO | ✅ PASS | 编写 `tests/test_telemetry_store_crash.c`（4/4 测试通过） |
| **手术级修改 (Surgical Edit)** | 仅改动目标模块 | ✅ PASS | 仅深化 `modules/edge_infra/storage/edge_telemetry_store.c` 与 SDK 头文件 |
| **反熵核心只读门禁** | `STRICT_CORE_GATE=1 check_core_readonly.sh` | ✅ PASS | 0 违规核心修改，核心只读门禁通过 |
| **反熵符号基线门禁** | `STRICT_CORE_GATE=1 check_core_symbols.sh` | ✅ PASS | `libedge_core.a` 0 外部禁止符号 |
| **二进制尺寸门禁** | `STRICT_SIZE_GATE=1 check_binary_sizes.sh` | ✅ PASS | 4/4 产品二进制严控尺寸预算通过 |
| **生成物可信链门禁** | `STRICT_DRIFT_GATE=1 check_generated_drift.sh` | ✅ PASS | 编译器一致(delta:0B)，全生成物零漂移 |
| **独立分支与证据包** | ticket/ARCH-DEEP-03, summary/tests/redlines/diffstat | ✅ PASS | 独立分支 `ticket/ARCH-DEEP-03`，全量 `tests.log` 实测入库 |
