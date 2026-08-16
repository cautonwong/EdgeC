# AEP-204 证据包总结 (Summary) - Rework Round 2

- **工单 ID**: AEP-204 (Issue #82)
- **标题**: edge_core 依赖白名单收窄 + nm -u 门禁 + DEV 回退测试与架构闭环
- **分支**: `ticket/AEP-204`
- **状态**: Done (Rework Round 2 Completed - Ready for Review)
- **完成时间**: 2026-08-16

---

## 1. 变更内容与改动说明

1. **`edge_core` 外部依赖收窄与 `nm -u` 复核**:
   - 从 `modules/edge_core/CMakeLists.txt` 中彻底移除 `cyclonedds` 依赖，CycloneDDS 仅保留在 `edge_infra/bus_dds`；
   - `edge_core` 私有保留 `cjson`（用于 `edge_app_mgr.c` 容器/应用描述符解析），并在白名单中明确声明；
   - `nm -u` 严苛复核：`libedge_core.a` 对 `sqlite3`、`paho`、`dds`、`UA` 等底层引擎的符号依赖绝对为 0。

2. **严格门禁接入 CI (`.github/workflows/ci.yml`)**:
   - 在 `.github/workflows/ci.yml` 的架构门禁步骤中正式接入：
     - `STRICT_CORE_GATE=1 ./scripts/check_core_symbols.sh`
     - `STRICT_CORE_GATE=1 ./scripts/check_core_readonly.sh HEAD~1`

3. **真实定向单测（覆盖 `secure_syslog_exporter.c` 与 `stage_vpp_logger.c`）**:
   - 彻底删除只测 getenv/setenv 的假测试；
   - 在 `tests/test_iems_task_loader.c` 中真实调用产品函数：
     - 调用 `stage_vpp_logger_create(svc)` 验证 `EDGE_ENV=production` 无 secret 时 Fail-Fast 返回 `NULL`，开发模式安全回退；
     - 调用 `dpi_audit_signed_log(DPI_PASS/DPI_BLOCK_RANGE, ...)` 验证开发与生产环境下签名日志真实输出。

4. **IEMS 任务装载复用 `g_app.db_service` 与产品代码直连**:
   - 创建 [cmd/iems/iems_task_loader.h](file:///workspaces/edgeos/EdgeC/cmd/iems/iems_task_loader.h) 与 [cmd/iems/iems_task_loader.c](file:///workspaces/edgeos/EdgeC/cmd/iems/iems_task_loader.c)；
   - 在 `cmd/iems/main.c` 中彻底消除 `sqlite3_open_v2` 双路径，统一复用 `g_app.db_service`；
   - `tests/test_iems_task_loader.c` 直连测试 `iems_poll_tasks_load` 产品函数，验证元数据未就绪拦截与多总线任务装载。

5. **SHM 硬断言归属说明**:
   - SHM 性能压测与并发硬断言按派工计划书面归属至 **AEP-208**（全链路压测与最终加固里程碑）统一落地。

6. **全量测试与门禁验证**:
   - `ctest --test-dir build -j1`: **136/136 100% Passed (47.16s)**；
   - `./test.sh --omnis`: **10/10 Passed (55.36s)**；
   - `go test ./...`: 100% Passed；
   - `check_no_old_src.sh`: PASS；
   - `check_symbols_baseline.sh`: PASS；
   - `STRICT_CORE_GATE=1 check_core_symbols.sh`: PASS；
   - `STRICT_CORE_GATE=1 check_core_readonly.sh`: PASS。

---

## 2. DoD 逐项验收勾选

- [x] `edge_core/CMakeLists.txt` 彻底移除 `cyclonedds`，nm -u 验证 0 违规符号
- [x] `.github/workflows/ci.yml` 接入 `STRICT_CORE_GATE=1` 的 core_symbols 与 core_readonly 门禁
- [x] `tests/test_iems_task_loader.c` 真实测试 `stage_vpp_logger_create` 与 `dpi_audit_signed_log` 的 EDGE_ENV 守卫
- [x] `cmd/iems/main.c` 消除 `sqlite3_open_v2` 双路径，统一复用 `g_app.db_service`
- [x] 提取 `iems_task_loader.c` 可测函数，单测直连产品代码
- [x] SHM 硬断言书面明确归属 AEP-208
- [x] 136/136 CTest -j1 + 10/10 Omnis + Go 测试全绿
- [x] 真实证据日志入库
