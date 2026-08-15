# Codex 四审：可执行性评审与 Ticket 拆分（Execution Readiness）

> **Mr. WKT** — 四审不再讨论“方向对不对”（R1~R3 已定论），只回答一个问题：**AGY 拿到这份主计划，能不能直接开工？** 结论：不能直接开工——缺输入输出、缺验收命令、缺依赖排序，且有 3 处与仓库现状冲突。本审给出可直接执行的 AEP Ticket 清单。

---

## 0. 判定

**可执行性评级：C+（需修订后开工）。**

主计划是合格的“阶段路线图”，但不是合格的“执行计划”：

1. 每个 Task 只有一句话，无输入/输出/验收命令/依赖/工时；
2. 与仓库现状有 3 处直接冲突（见 §2）；
3. 未与既有 41 Ticket 路线图（T-201~T-206 等）去重合并；
4. 未定义 Phase 0（基线冻结），而当前工作区有约 25 个未提交新文件。

本审输出：**AEP-001 ~ AEP-510 共 48 个可执行 Ticket**（含 DoD、依赖、估时）。

---

## 1. 阶段可执行性评分

| 阶段 | 输入明确 | DoD 可验证 | 依赖明确 | 当前阻塞 | 评分 |
|---|---|---|---:|---|
| Phase 0（缺失） | — | — | — | 未提交 WIP 25+ 文件未冻结 | F |
| Phase 1 双重源码树 | ✅ | ✅ | ✅ | 低 | A- |
| Phase 2 核心解耦 | ⚠️ 部分 | ⚠️ 无依赖/尺寸门禁 | ⚠️ 35 文件依赖面未提 | 中 | C+ |
| Phase 3 自注册+CLI | ⚠️ 机制未定 | ⚠️ 无 nm/顺序 DoD | 依赖 Phase 2 | 中 | C |
| Phase 3.5（缺失） | — | — | — | 无 DDD 任务 | F |
| Phase 4 Out-of-Tree | ⚠️ 无 ABI/许可/依赖策略 | ⚠️ 无独立 CI 验证 | 依赖 Phase 3 | 中高 | C |
| Phase 5（缺失） | — | — | — | 无长期治理机制 | F |

## 2. 与仓库现状的 3 处冲突（开工前必须修正）

1. **路径错误**：主计划 Task 3.1 写“在 `include/edge_driver.h` 实现 `EDGE_DRIVER_REGISTER`”，但该头文件实际在 `modules/edge_io/include/edge_driver.h`（`include/` 下不存在）。
2. **旧 src 树性质未说清**：`src/CMakeLists.txt` 仍定义 `edge_sdk`，但顶层 CMake 已不再 `add_subdirectory(src)`——它已是死代码；Phase 1 不是“清理双重源码树”，而是“删除死树 + 迁移 4 个仍被引用的文件”（`northbound_mgr.c`、`sched_engine.c`、`sched_port.c`、`io_executor_linux.c`）。
3. **基线漂移**：工作区未提交内容包含 DCU 12 个新模块、IEMS 7 个新模块、`edge_bsp_hal`、SunSpec 绑定等约 25 个文件；计划声称的 134/134 + 10/10 基线“现在”并不存在一个冻结快照。

另：`docs/agents/issue-tracker.md` 已切到 GitHub Issues（gh CLI）。是否真建 Issue 由 Mr. WKT 定；以下 Ticket 先以文档为准，编号 `AEP-xxx` 可与既有 T-xxx 映射。

---

## 3. Ticket 拆分

### Phase 0：基线冻结与止血（P0，2-4 人天）

| ID | 任务 | 依赖 | DoD（可验证） | 估时 |
|---|---|---|---:|
| AEP-001 | 冻结并收口 WIP：DCU/IEMS 新模块、edge_bsp_hal、SunSpec 决定“提交/回退/挂起” | 无 | `git status` 干净或清单化；基线 tag `baseline-2026-08-16` | 0.5d |
| AEP-002 | 复跑并归档基线证据：build/ctest(134)/omnis(10)/consistency/hygiene | AEP-001 | 证据文件 + CI 绿 | 0.5d |
| AEP-003 | 修复 CI 参数：`-DENABLE_COVERAGE=ON` → `-DCOVERAGE=ON`；确认 ASan/TSan 矩阵真实生效 | AEP-002 | CI 覆盖 job 产出 gcda；ASan job 跑完 134 | 0.5d |
| AEP-004 | R3 未闭合项收口：T-002 剩余（HMAC 外置、DataHub 回写、去硬编码 DTC、补 Go 测试） | 无 | `go test` 新增用例全绿；`rg` 无硬编码密钥 | 1d |
| AEP-005 | 把 R1~R4 评审意见登记为 backlog（映射既有 T-xxx） | 无 | 单一登记表 | 0.5d |
| AEP-006 | 建立“核心目录只读”CI 门禁原型（CODEOWNERS + 路径检查） | AEP-001 | 改动 `modules/edge_core` 的 PR 被标记 | 0.5d |

### Phase 1：双重源码树清理（P1，3-5 人天）

| ID | 任务 | 依赖 | DoD（可验证） | 估时 |
|---|---|---|---:|
| AEP-101 | 迁移 4 个被引用文件到 `modules/edge_io/src/`（含 include 修正） | AEP-001 | `rg 'src/main_loop|src/platform'` 无命中；构建绿 | 1d |
| AEP-102 | 清理 `modules/edge_io/CMakeLists.txt` 跨目录引用 | AEP-101 | 无 `${CMAKE_SOURCE_DIR}/src`；CTest 134 | 0.5d |
| AEP-103 | 核查 `src/` 其余文件是否死代码（core/stream_engine/main_loop 等） | AEP-101 | 引用分析报告 | 0.5d |
| AEP-104 | 删除死树 `src/`（含 `src/CMakeLists.txt`） | AEP-103 | 删除后 build+ctest+omnis 全绿 | 0.5d |
| AEP-105 | 加回归门禁：`rg` 路径检查 + `nm -g` 关键符号清单对比 | AEP-104 | CI job 存在且失败即阻断 | 0.5d |

### Phase 2：核心解耦与 edge_infra（P1，10-15 人天）

| ID | 任务 | 依赖 | DoD（可验证） | 估时 |
|---|---|---|---:|
| AEP-201 | 定义 `storage_ops_t`、`datahub_ops_t` 与注入点（main.c/init） | AEP-105 | 头文件 + 接口评审记录 | 1d |
| AEP-202 | 迁移 `sqlite_service` 到 `modules/edge_infra/storage/`（保留 WAL/SnF 语义） | AEP-201 | ADR-0007 专项回归绿 | 2-3d |
| AEP-203 | 迁移 `datahub_dds.c`/`datahub_mqtt.c` 到 `edge_infra/bus_*`（保留 vendored fallback） | AEP-201 | 构建绿；DDS/MQTT 测试绿 | 2d |
| AEP-204 | `edge_core` 依赖白名单落地（仅 libuv + 必要日志） | AEP-202/203 | `nm -u` 门禁通过 | 1d |
| AEP-205 | 尺寸门禁：定义口径并加 CI（WARN/ERROR 两档） | AEP-204 | CI artifact 含尺寸报告 | 0.5d |
| AEP-206 | 清理 35 个 sqlite 调用点的 include/链接 | AEP-202 | 无旧路径引用 | 1d |
| AEP-207 | Kconfig 重排：产品按需链 `edge_infra/*` | AEP-204 | 三种产品配置均可构建 | 1d |
| AEP-208 | WAL/压实/SnF/DCU 专项回归 + Omnis 10/10 | AEP-202~207 | 全绿 + 证据 | 1d |

### Phase 3：生成式注册与 edge-cli（P1，5-10 人天）

| ID | 任务 | 依赖 | DoD（可验证） | 估时 |
|---|---|---|---:|
| AEP-301 | 编译器新增 driver registry 生成（YAML 声明 → `edge_driver_registry.c`） | AEP-204 | 新增驱动零核心改动；`nm` 断言注册表完整 | 2-3d |
| AEP-302 | 迁移 iems/dcu 手工 extern 注册为生成式注册表 | AEP-301 | main.c 无 `extern ..._register`；omnis 绿 | 1d |
| AEP-303 | `edge-cli create-product/driver/lint-contract`（复用 compiler-v2 validator） | AEP-301 | 模板可直接编译运行 | 2d |
| AEP-304 | DX 集成测试：新建驱动→YAML→注册→被调度 | AEP-303 | `tests/test_dx_scaffolding.c` 绿 | 1d |
| AEP-305 | 生成物可信链：CI 重生成比对，手改=失败 | AEP-301 | 故意手改生成物→CI 红 | 0.5d |

### Phase 3.5：领域建模与契约治理（P1，5 人天，可与 Phase 3 并行）

| ID | 任务 | 依赖 | DoD（可验证） | 估时 |
|---|---|---|---:|
| AEP-351 | 建立 `CONTEXT-MAP.md` + 每上下文 `CONTEXT.md`（仅词汇） | 无 | 四上下文词汇表 | 1d |
| AEP-352 | TagID 分域与界限上下文绑定（复用 T-202） | AEP-351 | validator 拒绝跨域 Tag | 1d |
| AEP-353 | ACL 层模块：寄存器→领域实体映射 | AEP-352 | 禁止原始地址穿透用例 | 1-2d |
| AEP-354 | 每上下文 2 个核心场景用例（DCU 冻结抄读、IEMS 孤岛等） | AEP-351 | Matrix 用例绿 | 1d |
| AEP-355 | 编译器/validator 补“跨产品同名 Tag”冲突检测 | AEP-352 | 冲突契约被拒 | 0.5d |

### Phase 4：Out-of-Tree SDK（P2，5-10 人天）

| ID | 任务 | 依赖 | DoD（可验证） | 估时 |
|---|---|---|---:|
| AEP-401 | `cmake/EdgeOSConfig.cmake.in` + `install(EXPORT EdgeOSTargets)` | AEP-204 | 安装后 `find_package(EdgeOS)` 可用 | 1-2d |
| AEP-402 | `examples/out_of_tree_bess/` 独立项目 | AEP-401 | 仅用安装包编译运行 | 1d |
| AEP-403 | SDK 依赖与许可清单（SBOM + 许可审查） | 无 | 清单文档 | 1d |
| AEP-404 | ABI golden 测试（PipeMsg 64B、edge_tag_meta_t、ops 虚表） | AEP-401 | golden header 比对 CI | 1d |
| AEP-405 | `edge-cli migrate` 与破坏性变更清单规范 | AEP-303 | 迁移演示用例 | 1-2d |
| AEP-406 | 《EdgeOS 二开工程师手册》 | AEP-402 | 手册发布 | 1d |

### Phase 5：长期治理机制（P2，持续）

| ID | 任务 | 依赖 | DoD（可验证） | 估时 |
|---|---|---|---:|
| AEP-501 | ADR 生命周期治理（Proposed→Accepted→Superseded + 年度审计） | 无 | 流程文档 + 模板 | 0.5d |
| AEP-502 | API/ABI 兼容政策（semver + 破坏性变更清单） | AEP-404 | 政策文档 + golden CI | 1d |
| AEP-503 | 熵指标与年度《防腐健康报告》 | AEP-105 | 报告模板 + 首期报告 | 1d |
| AEP-504 | CODEOWNERS + 核心目录双人评审门禁（不可豁免，豁免留痕） | AEP-006 | 门禁生效 | 0.5d |
| AEP-505 | LTS 与设备升级/回滚矩阵 | 无 | 矩阵文档 | 0.5d |
| AEP-506 | 工具链容器化 + 依赖 EOL 日历 | 无 | 构建镜像 + 日历 | 1d |
| AEP-507 | SBOM/安全扫描/密钥轮换管线（接 T-001） | 无 | CI 扫描 job | 1d |
| AEP-508 | docs-as-code 强制（文档随 PR 更新） | 无 | PR 模板检查 | 0.5d |
| AEP-509 | 防表演式全绿：核心文件覆盖率 + 变异测试抽查 | AEP-003 | 门禁示例 | 1d |
| AEP-510 | 知识传承：Playbook + 轮岗 + 决策记录规范 | 无 | 手册 | 0.5d |

---

## 4. 依赖图与里程碑

```
AEP-001~006 (Phase 0) ──► AEP-101~105 (P1) ──► AEP-201~208 (P2)
                        │                        │
                        ├──► AEP-351~355 (P3.5) ◄┘（并行）
                        └──► AEP-301~305 (P3) ──► AEP-401~406 (P4)
                                                    │
                              AEP-501~510 (P5) ◄───┘（持续）
```

里程碑：M0=基线冻结；M1=src 清零；M2=core 纯净；M3=二开闭环；M4=长期治理上线。

工期粗估（2-3 人）：M0 0.5w；M1 0.5w；M2 2-3w；M3 1-2w；M4 1w；P5 持续。合计 6-8 周。

## 5. 执行风险 FMEA（Top 10）

| 失效模式 | 原因 | 影响 | 现有控制 | RPN | 建议动作 |
|---|---|---|---|---:|---|
| 重构混入功能开发 | WIP 未冻结 | 基线漂移、回归难定位 | 无 | 高 | AEP-001 先行 |
| 移动文件丢符号 | 头文件路径遗漏 | 链接失败 | CTest | 中 | `nm` 对比门禁 |
| sqlite 迁移破坏 WAL | 语义迁移不当 | 掉电丢数据 | ADR-0007 测试 | 高 | AEP-202 专项回归 |
| 自注册被链接器裁剪 | 无机制说明 | 驱动“注册了但不存在” | 无 | 高 | AEP-301 生成式方案 |
| 生成物被手改 | 无可信链 | 契约/注册表漂移 | 无 | 中 | AEP-305 |
| CI 表演式全绿 | 门禁太弱 | 腐化无感知 | 无 | 高 | AEP-003/509 |
| Out-of-tree 依赖缺失 | 安装包不完整 | 二开无法编译 | 无 | 中 | AEP-401/403 |
| 老 SDK 锁死 | 无迁移工具 | 二开分裂 | 无 | 高 | AEP-405 |
| 关键人离职 | 知识未沉淀 | 无人敢重构 | 无 | 高 | AEP-510 |
| Tag 跨产品冲突 | 无全局查重 | 数据语义错乱 | validator 单产品 | 中 | AEP-352/355 |

## 6. 与既有 41 Ticket 的映射（避免重复）

| 既有 | 对应 AEP |
|---|---|
| T-201 编译器校验 | AEP-303/355 |
| T-202 TagID 命名空间 | AEP-352 |
| T-204 C/Go 单源 | AEP-305 |
| T-205 Blob 签名/热切换 | AEP-507（部分） |
| T-206 V1 退役 | 已完成（R3 确认） |
| T-107 ASan/TSan | AEP-003 |
| T-002 UDS/DoIP | AEP-004 |

建议：以 AEP 为新编号，T 系列作为历史引用；合并时在登记表保留双向映射。

## 7. 结论与批准建议

1. 主计划方向在 R1~R3 已确认；四审证明它**缺少执行层**。
2. 批准建议：**先批 AEP-001~006（Phase 0）+ AEP-101~105（Phase 1）**，共 11 个 Ticket；Phase 2 起每批 5-8 个 Ticket 独立验收。
3. AGY 开工前必须修 3 处冲突（§2）并确认 Issue 载体（GitHub Issues 或文档）。

---

*四审完成：48 个 Ticket、依赖图、FMEA、映射表均可直接用于派工。*
