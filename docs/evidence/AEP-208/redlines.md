# AEP-208 红线自查与工程宪法核对 (Redlines Check)

> **基准宪法**: `GEMINI.md` / `CLAUDE.md` / `docs/plans/2026-08-16-aep-phase2-dispatch.md`

| 条目 | 宪法/规范要求 | 执行与落地实测 | 结论 |
|---|---|---|:---:|
| **1. 老板协议 (Boss-First)** | 每一轮交互必须以 `Mr. WKT` 开头强制对齐焦点 | 100% 遵守，严格执行 | ✅ 合规 |
| **2. 物理真实性 (Physical Truth)** | 零 Mock 占位符，数据必须源自物理测试 | Go 侧 SHM 测试彻底消除软跳过，实施 16384 容量物理硬断言；C 侧实测 WAL/压实/GC 物理执行 | ✅ 合规 |
| **3. 手术级修改 (Surgical Edit)** | 仅修改必要行，不顺手大重构 | 10 个文件，仅 107 行插入，精准修复 Kconfig 解析、门禁与断言 | ✅ 合规 |
| **4. 复现与回归先行** | 必须有真实绿灯测试与完整日志支撑 | CTest -j1 136/136 + Omnis 10/10 + Go 全绿 + 5 大架构门禁全绿 | ✅ 合规 |
| **5. 四平面隔离与接口解耦** | 不在 edge_core 中直接引用第三方引擎 | nm -u libedge_core.a 保持 0 禁止符号 | ✅ 合规 |
| **6. 尺寸门禁自校验与 CI 管道安全** | 非 Release 模式严格拦截，CI 开启 pipefail | check_binary_sizes.sh 自校验 Release 模式，CI 管道启用 bash -eo pipefail | ✅ 合规 |
| **7. 真实证据包入库** | tests.log / summary.md / diffstat.txt 真实完整 | 证据包已完整落盘于 docs/evidence/AEP-208/ | ✅ 合规 |
