# AEP-353 纪律与架构红线自检清单 (Redlines)

| 红线检查项 | 约束标准 | 达标情况 | 证明依据 / 备注 |
|---|---|:---:|---|
| **Boss Protocol** | 每轮以 Mr. WKT 开头 | ✅ PASS | 全程强制遵循上下文焦点对齐 |
| **ACL 隔离与防腐性** | 规约点位与领域实体解耦 | ✅ PASS | `edge_acl.h` 与 `edge_acl.c` 落地，业务领域层 0 规约寄存器魔数依赖 |
| **物理真实性 (Physical Truth)** | 零 Mock / 精准物理量换算 | ✅ PASS | Inbound/Outbound 真实缩放（`scaler`）、偏移（`offset`）与数据类型转换 |
| **并发与内存安全** | 线程安全与零内存泄漏 | ✅ PASS | `pthread_mutex_t` 锁保护点位映射表，动态扩容无野指针，`edge_acl_destroy` 安全释放 |
| **手术级修改 (Surgical Edit)** | 不扩大改动范围 | ✅ PASS | 仅新增 `edge_acl` 模块、`test_acl_engine.c`、CMake 注册与证据包 |
| **反熵核心只读门禁** | `STRICT_CORE_GATE=1 check_core_readonly.sh` | ✅ PASS | 0 违规核心修改，核心只读门禁通过 |
| **反熵符号基线门禁** | `STRICT_CORE_GATE=1 check_core_symbols.sh` | ✅ PASS | `libedge_core.a` 0 外部禁止符号 |
| **二进制尺寸门禁** | `STRICT_SIZE_GATE=1 check_binary_sizes.sh` | ✅ PASS | 核心库 118.7KB（远低于 250KB 预算），全产品符合尺寸预算 |
| **生成物可信链门禁** | `STRICT_DRIFT_GATE=1 check_generated_drift.sh` | ✅ PASS | 编译器一致，全生成物零漂移 |
| **独立分支与证据包** | ticket/AEP-353, summary/tests/redlines/diffstat | ✅ PASS | 独立分支 `ticket/AEP-353`，全量 `tests.log` 实测入库 |
