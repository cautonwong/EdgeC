# AEP Phase 2 派工指令（AEP-201 ~ AEP-208）

> **Mr. WKT / Codex → AGY** — Phase 0+1 已全部验收关闭。Phase 2 目标：核心依赖解耦与 `edge_infra` 插件化。
> 生效规则：每票一个分支、一个 PR、一次 Codex 复审；未经复审不得合并。

---

## 1. 任务清单与依赖

| Issue | Ticket | 任务 | 依赖 | 估时 |
|---|---|---|---:|
| [#79](https://github.com/cautonwong/EdgeC/issues/79) | AEP-201 | 定义 `storage_ops_t` / `datahub_ops_t` 抽象虚表与注入点 | 无 | 1.0d |
| [#80](https://github.com/cautonwong/EdgeC/issues/80) | AEP-202 | 迁移 `sqlite_service` → `modules/edge_infra/storage/`（保留 WAL/SnF） | AEP-201 | 2.5d |
| [#81](https://github.com/cautonwong/EdgeC/issues/81) | AEP-203 | 迁移 `datahub_dds.c` / `datahub_mqtt.c` → `edge_infra/bus_*`（保留 vendored fallback） | AEP-201 | 2.0d |
| [#82](https://github.com/cautonwong/EdgeC/issues/82) | AEP-204 | `edge_core` 依赖白名单收窄 + `nm -u` 门禁 + dev 回退回归测试 | AEP-202/203 | 1.0d |
| [#83](https://github.com/cautonwong/EdgeC/issues/83) | AEP-205 | 尺寸门禁脚本（Release+strip 口径，WARN/ERROR） | AEP-204 | 0.5d |
| [#84](https://github.com/cautonwong/EdgeC/issues/84) | AEP-206 | 清理 35 个 SQLite 调用点的 include/链接 | AEP-202 | 1.0d |
| [#85](https://github.com/cautonwong/EdgeC/issues/85) | AEP-207 | Kconfig 重排：`CONFIG_EDGE_INFRA_SQLITE/DDS/MQTT`，三产品独立编译 | AEP-204 | 1.0d |
| [#86](https://github.com/cautonwong/EdgeC/issues/86) | AEP-208 | 专项韧性回归（WAL/压实/SnF）+ Omnis 10/10 | AEP-202~207 | 1.0d |

**推荐执行顺序**：AEP-201 →（202、203 可并行）→ 206 → 204 →（205、207 可并行）→ 208。

## 2. Phase 2 特别红线（在 R6 红线之上）

1. **接口先行**：AEP-201 未过 Codex 复审，禁止开工 AEP-202/203（先定义虚表与注入点，再搬文件）。
2. **搬文件必须验符号**：每次迁移后运行 `check_symbols_baseline.sh`，`nm -g` 差异为 0；禁止“能编译但符号丢”。
3. **WAL/SnF 语义不可变**：`sqlite_service` 迁移必须保留 ADR-0007 的 WAL/快照/压实/断线缓存语义；AEP-208 专项不过不得宣布完成。
4. **edge_core 白名单机械验证**：`scripts/check_core_symbols.sh` 用 `nm -u` 校验；CI 开启 `STRICT_CORE_GATE=1`。
5. **禁止手改生成物**：任何迁移引发的契约/注册表/IDL 变化必须由工具链重新生成并提交 diff。
6. **证据诚实**：`summary.md` 必须与代码一致，禁止夸大；发现不符按红线处理。
7. **SHM 测试硬断言**：承接 Phase 1 note，`TestUDSStateSHMReadback` 在 CI 中不得软跳过（无 SHM 环境应显式失败或标记 skip 原因）。
8. **dev 回退测试补齐**：`DEV_ONLY_*`/`TEST_ONLY_*` 必须带 `EDGE_ENV != production` 守卫并有回归测试（并入 AEP-204 DoD）。

## 3. 每票验收门禁（DoD 摘要）

| Ticket | DoD（可验证） |
|---|---|
| AEP-201 | `include/` 或 `modules/edge_infra/include/` 落盘 `storage_ops.h`/`datahub_ops.h`；接口评审通过；无实现混入 |
| AEP-202 | sqlite_service 在 `edge_infra/storage/`；ADR-0007 专项回归绿；旧路径引用为 0 |
| AEP-203 | dds/mqtt 后端在 `edge_infra/bus_*`；vendored fallback 保留；单元测试绿 |
| AEP-204 | `edge_core/CMakeLists.txt` 仅保留白名单依赖；`nm -u` 无 SQLite/MQTT/DDS/cJSON；dev 回退测试绿 |
| AEP-205 | CI 产出 `libedge_core.a` 尺寸报告（Release+strip）；WARN/ERROR 阈值生效 |
| AEP-206 | `rg sqlite_service` 无旧 include/链接；CTest 134 保持 |
| AEP-207 | iems/dcu/smart_box 三种配置独立构建通过；Kconfig 文档同步 |
| AEP-208 | WAL 压实专项 + SnF 专项 + Omnis 10/10 + 全量 134 全绿，证据归档 |

## 4. 证据包与流程（与 Phase 0/1 相同）

```text
docs/evidence/AEP-2xx/summary.md
docs/evidence/AEP-2xx/tests.log
docs/evidence/AEP-2xx/redlines.md
docs/evidence/AEP-2xx/diffstat.txt
```

- 分支：`ticket/AEP-2xx`；提交信息含 AEP 编号；
- PR：指向 `feature/modular-refactor`，附证据包链接；
- 复审：Codex 按红线宪章验收，Pass 后才允许合并；
- 阻塞：Issue 留言标记 `needs-info`，不得自行扩权。

## 5. 给 AGY 的交代话术（可直接转发）

> AGY 你好，Phase 0+1 已验收关闭，现在进入 Phase 2（AEP-201~208，核心解耦与 edge_infra）。请严格按 `docs/plans/2026-08-16-aep-phase2-dispatch.md` 执行：
>
> 1. 先做 AEP-201（接口定义），未过 Codex 复审前禁止搬任何文件；
> 2. AEP-202/203 可并行，但必须保留 WAL/SnF 语义与 vendored fallback，且每次迁移后 `nm -g` 符号差异为 0；
> 3. 每票一个分支 `ticket/AEP-2xx`、一个 PR、一份证据包；未复审不得合并，禁止直接提交到 feature/modular-refactor；
> 4. 不手改生成物、不 Mock/假数据、不削弱门禁；CI 红不合并；
> 5. SHM 测试改为硬断言；DEV_ONLY/TEST_ONLY 回退补测试（并入 AEP-204）；
> 6. 全部完成后跑 AEP-208 专项（WAL/压实/SnF + Omnis 10/10 + CTest 134）并归档证据；
> 7. 阻塞立即在 Issue 留言，不要自行扩大范围或改 DoD。

---

*Phase 2 派工完毕。Codex 将按红线宪章逐票验收。*
