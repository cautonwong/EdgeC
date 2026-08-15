# EdgeOS Ticket 映射与评审意见双向追踪表 (TICKET_MAPPING.md)

> 本文档为 AEP 体系（AEP-001 ~ AEP-516）、历史 T 系列工单（T-001 ~ T-206）以及 Codex 六轮评审意见（R1 ~ R6）的双向对照与生命周期追踪表。

---

## 1. AEP 工单 ↔ 历史 T 系列工单 ↔ R1~R6 评审映射矩阵

| AEP 工单 | 阶段 | 标题 | 对应历史工单 | 对应评审意见 | 状态 |
|---|---|---|---|---|---|
| **AEP-001** | Phase 0 | 收口未提交 WIP | — | R1 §0, R4 §2, R5 §2.2 | ✅ Mr. WKT 决策全部提交 |
| **AEP-002** | Phase 0 | 基线证据归档 | — | R1 §0, R3 §2, R4 §3 | Ready |
| **AEP-003** | Phase 0 | 修复 CI 参数 (-DCOVERAGE=ON) | T-107 | R1 §5.7, R2 §6, R4 §3 | Ready |
| **AEP-004** | Phase 0 | R3 未闭合项收口 (T-002 UDS/HMAC) | T-002 | R3 P0-3, R4 §3, R5 §1 | ✅ 已就绪 |
| **AEP-005** | Phase 0 | 评审意见双向映射表 | — | R4 §3, R5 §1 | ✅ 本文档 |
| **AEP-006** | Phase 0 | 核心目录只读门禁原型 | — | R3 §5.5, R4 §3 | Ready |
| **AEP-101** | Phase 1 | 平移 4 个存活文件 | — | R1 §4, R2 §1, R4 §2 | Ready |
| **AEP-102** | Phase 1 | 重构 edge_io CMake | — | R1 §4, R4 §3 | Ready |
| **AEP-103** | Phase 1 | 死代码审计报告 | — | R1 §4, R4 §2 | Ready |
| **AEP-104** | Phase 1 | 删除旧 src/ 目录 | — | R1 §4, R4 §3 | Ready |
| **AEP-105** | Phase 1 | 路径与符号回归门禁 | — | R1 §4, R2 §1, R4 §3 | Ready |
| **AEP-201** | Phase 2 | 定义 storage_ops_t / datahub_ops_t | — | R1 §3.3, R2 §2, R4 §3 | Backlog |
| **AEP-202** | Phase 2 | 迁移 sqlite_service 到 edge_infra | — | R1 §3.3, R2 §2, R4 §3 | Backlog |
| **AEP-203** | Phase 2 | 迁移 datahub_dds/mqtt 到 edge_infra | — | R1 §3.2, R4 §3 | Backlog |
| **AEP-204** | Phase 2 | 内核依赖白名单收窄 (仅 libc+uv+zlog)| — | R1 §3.2, R2 §2, R4 §3 | Backlog |
| **AEP-205** | Phase 2 | 尺寸门禁脚本落地 (150KB 门禁) | — | R1 §3.2, R2 §5, R4 §3 | Backlog |
| **AEP-206** | Phase 2 | 清理 35 个 SQLite 调用点 | — | R2 §1, R4 §3 | Backlog |
| **AEP-207** | Phase 2 | Kconfig 重排 | — | R4 §3 | Backlog |
| **AEP-208** | Phase 2 | 专项韧性回归 (WAL/SnF/Omnis) | — | R1 §3.3, R4 §3 | Backlog |
| **AEP-301** | Phase 3 | 编译器生成驱动注册表 | — | R2 §3, R3 §3, R4 §3 | Backlog |
| **AEP-302** | Phase 3 | 消灭手工 extern 注册 | — | R2 §3, R4 §3 | Backlog |
| **AEP-303** | Phase 3 | edge-cli 脚手架 (集成 validator) | T-201 | R1 §4.1, R4 §3 | Backlog |
| **AEP-304** | Phase 3 | DX 自动化集成测试 | — | R2 §3, R4 §3 | Backlog |
| **AEP-305** | Phase 3 | 生成物可信链校验 (防手改) | T-204 | R3 §5.3, R4 §3 | Backlog |
| **AEP-351** | Phase 3.5| 建立 CONTEXT-MAP 与领域词汇 | — | R2 §4, R3 §5.9, R4 §3 | Backlog |
| **AEP-352** | Phase 3.5| TagID 兼容分域与别名映射 | T-202 | R1 §3.5, R2 §4, R5 §2.3 | Backlog |
| **AEP-353** | Phase 3.5| ACL 防腐层模块落地 | — | R1 §3.5, R2 §4, R4 §3 | Backlog |
| **AEP-354** | Phase 3.5| 核心业务场景用例固化 | — | R2 §4, R4 §3 | Backlog |
| **AEP-355** | Phase 3.5| 跨产品同名 Tag 冲突查重 | T-201 | R3 §3, R4 §3 | Backlog |
| **AEP-401** | Phase 4 | 导出 CMake Targets (find_package) | — | R1 §3.4, R4 §3 | Backlog |
| **AEP-402** | Phase 4 | 独立二开示例工程 | — | R1 §3.4, R4 §3 | Backlog |
| **AEP-403** | Phase 4 | SDK 依赖与许可审查 (SBOM) | — | R1 §3.4, R3 §1.8, R4 §3 | Backlog |
| **AEP-404** | Phase 4 | ABI Golden Header 回归断言 (64B) | — | R1 §3.4, R3 §5.2, R4 §3 | Backlog |
| **AEP-405** | Phase 4 | edge-cli migrate 跨版本迁移工具 | — | R3 §4, R4 §3 | Backlog |
| **AEP-406** | Phase 4 | 编制《EdgeOS 二开工程师手册》 | — | R4 §3 | Backlog |
| **AEP-501** | Phase 5 | ADR 生命周期治理规范 | — | R3 §5.1, R4 §3 | Backlog |
| **AEP-502** | Phase 5 | API/ABI 兼容性发布政策 (SemVer) | — | R3 §5.2, R4 §3 | Backlog |
| **AEP-503** | Phase 5 | 年度防腐健康审计工具 | — | R3 §5.4, R4 §3 | Backlog |
| **AEP-504** | Phase 5 | CODEOWNERS 与双人评审门禁 | — | R3 §5.5, R4 §3 | Backlog |
| **AEP-505** | Phase 5 | LTS 分支与固件升级矩阵 | — | R3 §5.6, R4 §3 | Backlog |
| **AEP-506** | Phase 5 | 工具链容器化与 EOL 日历 | — | R3 §5.7, R4 §3 | Backlog |
| **AEP-507** | Phase 5 | 安全扫描与密钥轮换管线 | T-001, T-205 | R3 §5.8, R4 §3 | Backlog |
| **AEP-508** | Phase 5 | Docs-as-Code 强制机制 | — | R3 §5.9, R4 §3 | Backlog |
| **AEP-509** | Phase 5 | 防表演式全绿门禁 (核心覆盖率) | — | R3 §5.10, R4 §3 | Backlog |
| **AEP-510** | Phase 5 | 知识传承制度与 Playbook | — | R3 §5.9, R4 §3 | Backlog |
| **AEP-511** | Phase 5 | ADR-0008 V5 治理宪章闭环 | — | R5 §2.1 | Ready |
| **AEP-512** | Phase 5 | 产品合规矩阵 (IEC 62304/62443/MISRA)| — | R5 §2.4 | Backlog |
| **AEP-513** | Phase 5 | 需求追踪矩阵 (RTM) 工具 | — | R5 §2.4 | Backlog |
| **AEP-514** | Phase 5 | 性能基线与资源预算门禁 | — | R5 §2.5 | Backlog |
| **AEP-515** | Phase 5 | 二开上游贡献与同步规范 | — | R5 §3.3 | Backlog |
| **AEP-516** | Phase 5 | MISRA C:2012 静态扫描试点 | — | R5 §2.4 | Backlog |

---

## 2. 状态定义

- **Ready**: 依赖已满足，可直接排期/指派执行。
- **In Progress**: 正在执行中。
- **Done**: 满足通用 DoD 且证据包已落盘，经评审确认完成。
- **Backlog**: 待前置阶段出口 Gate 全绿后解锁。
