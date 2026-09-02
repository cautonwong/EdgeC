# ARCH-DEEP-02 纪律与架构红线自检清单 (Redlines)

| 红线检查项 | 约束标准 | 达标情况 | 证明依据 / 备注 |
|---|---|:---:|---|
| **Boss Protocol** | 每轮以 Mr. WKT 开头 | ✅ PASS | 全程强制遵循上下文焦点对齐 |
| **南向自动轮询编排** | 驱动管理器内建自动化调度与生命周期管线 | ✅ PASS | 实现 `driver_manager_start_auto_polling`, `driver_manager_add_poll_target`, `driver_manager_feed_response` |
| **挂钟 MTT 时间对齐** | 整点与毫秒级挂钟时间边界对齐调度 | ✅ PASS | 实现 `_now_ms()` 与整除边界 drift 补偿算法 |
| **缓冲区零泄漏保证** | 统一接管 `net_buf_t` 分配、编码、传输与回收 | ✅ PASS | 测试验证异常/超时/成功全链路 net_buf 零泄漏 |
| **超时与熔断退避** | 物理信道超时自动重试与指标上报 | ✅ PASS | 实现 `edge_poll_stats_t` 与超时熔断计数 |
| **物理真实性 (Physical Truth)** | 无 Mock、真实内存映射与物理断言 | ✅ PASS | 编写 `tests/test_driver_orchestrator.c`（3/3 测试通过） |
| **手术级修改 (Surgical Edit)** | 不扩大改动范围 | ✅ PASS | 仅深化 `modules/edge_io/src/driver_manager.c` 与 SDK 头文件 |
| **反熵核心只读门禁** | `STRICT_CORE_GATE=1 check_core_readonly.sh` | ✅ PASS | 0 违规核心修改，核心只读门禁通过 |
| **反熵符号基线门禁** | `STRICT_CORE_GATE=1 check_core_symbols.sh` | ✅ PASS | `libedge_core.a` 0 外部禁止符号 |
| **二进制尺寸门禁** | `STRICT_SIZE_GATE=1 check_binary_sizes.sh` | ✅ PASS | 4/4 产品二进制严控尺寸预算通过 |
| **生成物可信链门禁** | `STRICT_DRIFT_GATE=1 check_generated_drift.sh` | ✅ PASS | 编译器一致(delta:0B)，全生成物零漂移 |
| **独立分支与证据包** | ticket/ARCH-DEEP-02, summary/tests/redlines/diffstat | ✅ PASS | 独立分支 `ticket/ARCH-DEEP-02`，全量 `tests.log` 实测入库 |
