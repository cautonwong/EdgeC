# AEP-305 纪律与架构红线自检清单 (Redlines)

| 红线检查项 | 约束标准 | 达标情况 | 证明依据 / 备注 |
|---|---|:---:|---|
| **Boss Protocol** | 每轮以 Mr. WKT 开头 | ✅ PASS | 全程强制遵循上下文焦点对齐 |
| **可信链防手改** | 手改生成物阻断构建 | ✅ PASS | `check_generated_drift.sh` 自动重构比对，手改 100% 报错拦截退出 |
| **编译器可复现性 (Note 4)** | 源码编译与入库一致性 | ✅ PASS | 自动从 Go 源码构建编译器并校验尺寸与功能一致（delta: 0 bytes） |
| **物理真实性 (Physical Truth)** | 零 Mock / 零假数据 | ✅ PASS | 真实的 YAML $\rightarrow$ 编译器 $\rightarrow$ 查表与绑定，零假数据 |
| **手术级修改 (Surgical Edit)** | 不扩大改动范围 | ✅ PASS | 仅增加漂移门禁脚本、DoD 防篡改单测及 CTest 配置 |
| **反熵核心只读门禁** | `STRICT_CORE_GATE=1 check_core_readonly.sh` | ✅ PASS | 0 违规核心修改，核心只读门禁通过 |
| **反熵符号基线门禁** | `STRICT_CORE_GATE=1 check_core_symbols.sh` | ✅ PASS | `libedge_core.a` 0 外部禁止符号 |
| **二进制尺寸门禁** | `STRICT_SIZE_GATE=1 check_binary_sizes.sh` | ✅ PASS | 核心库 118.7KB（远低于 250KB 预算），全产品符合尺寸预算 |
| **独立分支与证据包** | ticket/AEP-305, summary/tests/redlines/diffstat | ✅ PASS | 独立分支 `ticket/AEP-305`，全量 `tests.log` 实测入库 |
