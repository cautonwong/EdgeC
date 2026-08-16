# AEP-205 证据包总结 (Summary)

- **工单 ID**: AEP-205 (Issue #83)
- **标题**: 尺寸门禁脚本落地（Release+strip 口径，WARN/ERROR 阈值与 CI 自动化）
- **分支**: `ticket/AEP-205`
- **状态**: Done (Ready for Review)
- **完成时间**: 2026-08-16

---

## 1. 变更内容与改动说明

1. **尺寸门禁脚本落盘 (`scripts/check_binary_sizes.sh`)**:
   - 编写落盘 [scripts/check_binary_sizes.sh](file:///workspaces/edgeos/EdgeC/scripts/check_binary_sizes.sh)；
   - 采用标准的 **Release + strip** 真实物理落地体积测量口径（`strip --strip-unneeded` 对 `.a` 静态库，`strip --strip-all` 对可执行二进制）；
   - **严格阈值设定**：
     - `libedge_core.a`: 剥离调试符号后体积为 **127.0 KB**（WARN: 250 KB, ERROR: 500 KB）
     - `iems`: 剥离体积 **3.64 MB**（WARN: 5.00 MB, ERROR: 8.00 MB）
     - `dcu`: 剥离体积 **2.61 MB**（WARN: 5.00 MB, ERROR: 8.00 MB）
     - `smart_box`: 剥离体积 **2.91 MB**（WARN: 5.00 MB, ERROR: 8.00 MB）
   - 支持 `STRICT_SIZE_GATE=1` 警告级强门禁模式。

2. **CI 流程与反熵门禁全链路接入 (`.github/workflows/ci.yml`)**:
   - 在 CI 流程的架构反熵门禁阶段接入 `./scripts/check_binary_sizes.sh`；
   - 将 CI 中 `check_core_readonly.sh` 基准严格对齐为 `origin/feature/modular-refactor`。

3. **结转 4 条 Notes 全部闭环**:
   - **Note ①**：`docs/evidence/AEP-204/diffstat.txt` 插入数精准修正为 522；
   - **Note ②**：在 `contracts/schema/core_readonly_whitelist.txt` 中正式落盘 `cjson` 依赖豁免说明（专用于 `edge_app_mgr.c` 容器描述符解析）；
   - **Note ③**：在 `tests/test_iems_task_loader.c` 中通过 `fork()` + `waitpid()` 进程隔离机制为 `dpi_audit_signed_log` 的 `EDGE_ENV=production` 缺失 secret 时的 `exit(EXIT_FAILURE)` 分支补充了 100% 真实断言单测；
   - **Note ④**：CI 中 `check_core_readonly.sh` 基准统一为 `origin/feature/modular-refactor`。

4. **全量测试与门禁验证**:
   - `ctest --test-dir build -j1`: **136/136 100% Passed (44.29s)**；
   - `./test.sh --omnis`: **10/10 Passed (57.96s)**；
   - `go test ./...`: 100% Passed；
   - 5 大反熵架构门禁全绿（`check_no_old_src.sh`, `check_symbols_baseline.sh`, `check_core_symbols.sh`, `check_core_readonly.sh`, `check_binary_sizes.sh`）。

---

## 2. DoD 逐项验收勾选

- [x] `scripts/check_binary_sizes.sh` 脚本落地，采用 Release+strip 测量口径
- [x] WARN/ERROR 阈值生效，核心库与三大产品体积 100% 达标
- [x] `.github/workflows/ci.yml` 正式接入尺寸门禁
- [x] `docs/evidence/AEP-204/diffstat.txt` 插入数修正为 522
- [x] `cjson` 依赖豁免在 `core_readonly_whitelist.txt` 中正式落盘
- [x] `dpi_audit_signed_log` 生产环境退出分支补充 fork 真实单测
- [x] CI 基准严格对齐 `origin/feature/modular-refactor`
- [x] 136/136 CTest -j1 + 10/10 Omnis + Go 测试全绿
- [x] 真实证据日志入库
