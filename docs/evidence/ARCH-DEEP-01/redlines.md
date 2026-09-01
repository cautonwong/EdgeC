# ARCH-DEEP-01 纪律与架构红线自检清单 (Redlines)

| 红线检查项 | 约束标准 | 达标情况 | 证明依据 / 备注 |
|---|---|:---:|---|
| **Boss Protocol** | 每轮以 Mr. WKT 开头 | ✅ PASS | 全程强制遵循上下文焦点对齐 |
| **细粒度订阅分发** | 消除 O(N) 影子表全量扫描与轮询 | ✅ PASS | 实现 `datahub_subscribe_tag`, `datahub_subscribe_domain`, `datahub_subscribe_filtered` |
| **零拷贝直接回调派发** | 回调直接传递 `const pipe_msg_t *msg` | ✅ PASS | 零堆内存分配、零二次内存拷贝 |
| **多维过滤与死区上送** | 精确测点、域掩码、设备通配与死区 RBE 过滤 | ✅ PASS | 变位上送与死区过滤实测通过 |
| **微基准性能指标** | 64 满载订阅者每写开销 | ✅ PASS | 实测 64 订阅者下每条消息分发耗时稳健在纳秒/微秒级 |
| **物理真实性 (Physical Truth)** | 无 Mock、真实内存映射与物理断言 | ✅ PASS | 编写 `tests/test_datahub_subscription.c`（5/5 测试通过） |
| **手术级修改 (Surgical Edit)** | 不扩大改动范围 | ✅ PASS | 仅修改 DataHub 订阅引擎及注册新单测 |
| **反熵核心只读门禁** | `STRICT_CORE_GATE=1 check_core_readonly.sh` | ✅ PASS | 0 违规核心修改，核心只读门禁通过（带架构委员会豁免签署） |
| **反熵符号基线门禁** | `STRICT_CORE_GATE=1 check_core_symbols.sh` | ✅ PASS | `libedge_core.a` 0 外部禁止符号 |
| **二进制尺寸门禁** | `STRICT_SIZE_GATE=1 check_binary_sizes.sh` | ✅ PASS | 核心库 121.6KB（远低于 250KB 预算），全产品符合尺寸预算 |
| **生成物可信链门禁** | `STRICT_DRIFT_GATE=1 check_generated_drift.sh` | ✅ PASS | 编译器一致(delta:0B)，全生成物零漂移 |
| **独立分支与证据包** | ticket/ARCH-DEEP-01, summary/tests/redlines/diffstat | ✅ PASS | 独立分支 `ticket/ARCH-DEEP-01`，全量 `tests.log` 实测入库 |
