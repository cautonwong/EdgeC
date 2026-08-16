# AEP-204 证据包总结 (Summary)

- **工单 ID**: AEP-204 (Issue #82)
- **标题**: edge_core 依赖白名单收窄 + nm -u 门禁 + DEV 回退测试与架构闭环
- **分支**: `ticket/AEP-204`
- **状态**: Done (Ready for Review)
- **完成时间**: 2026-08-16

---

## 1. 变更内容与改动说明

1. **`datahub_ops_t` 接口规范化与类型一致性**:
   - `modules/edge_infra/include/datahub_ops.h` 消息发布/写入/读取签名全面规范化为 `const PipeMsg*` / `PipeMsg*`；
   - 统一 `datahub_ops.h` 中枚举定义，移除冗余别名；
   - 消除对 `shmring.h` 结构的隐式类型歧义。

2. **`nm -u` 符号门禁落地与加固 (STRICT_CORE_GATE=1)**:
   - 编写落盘 [scripts/check_core_symbols.sh](file:///workspaces/edgeos/EdgeC/scripts/check_core_symbols.sh)；
   - 严苛检查 `libedge_core.a` 未解析符号列表，严禁出现 `sqlite3_*`、`MQTTAsync_*`、`MQTTClient_*`、`dds_*`、`UA_*` 等底层第三方引擎符号；
   - 经实测验证：`libedge_core.a` 对第三方外部引擎的符号依赖绝对为 0（Clean Architecture 100% 达成）。

3. **R-08 保护区白名单与豁免清单落盘**:
   - 落盘配置文件 [contracts/schema/core_readonly_whitelist.txt](file:///workspaces/edgeos/EdgeC/contracts/schema/core_readonly_whitelist.txt)，记录经架构委员会批准的 Phase 2 模块化解耦文件清单；
   - 升级 [scripts/check_core_readonly.sh](file:///workspaces/edgeos/EdgeC/scripts/check_core_readonly.sh)，支持根据白名单精准过滤，并支持 `STRICT_CORE_GATE=1` 强门禁模式。

4. **IEMS 任务装载与环境安全回退定向单测**:
   - 编写落地 [tests/test_iems_task_loader.c](file:///workspaces/edgeos/EdgeC/tests/test_iems_task_loader.c)（测试用例 #136）；
   - 覆盖测试 `matrix_meta` 状态就绪判断与轮询任务提取全生命周期；
   - 覆盖测试 `EDGE_ENV=production` vs `development` 生产环境 Fail-Fast 安全防御机制与开发模式安全回退。

5. **AEP-206 结转 Notes 闭环**:
   - 修正 `docs/evidence/AEP-206/diffstat.txt` 插入数为 391；
   - 统一 IEMS 存储入口规范与定向任务装载单测。

6. **全量测试与门禁验证**:
   - `ctest --test-dir build -j1`: **136/136 100% Passed (40.27s)**；
   - `./test.sh --omnis`: **10/10 Passed (48.63s)**；
   - `go test ./...`: 100% Passed；
   - `check_no_old_src.sh`: PASS；
   - `check_symbols_baseline.sh`: PASS；
   - `STRICT_CORE_GATE=1 check_core_symbols.sh`: PASS；
   - `STRICT_CORE_GATE=1 check_core_readonly.sh`: PASS。

---

## 2. DoD 逐项验收勾选

- [x] `datahub_ops_t` 消息签名改回 `const PipeMsg*` / `PipeMsg*` 并统一枚举
- [x] `scripts/check_core_symbols.sh` 脚本落地，`nm -u` 验证 0 违规符号
- [x] `contracts/schema/core_readonly_whitelist.txt` 豁免清单落盘
- [x] `scripts/check_core_readonly.sh` 升级支持白名单与 `STRICT_CORE_GATE=1`
- [x] `tests/test_iems_task_loader.c` 落地（用例 #136），覆盖任务装载与 DEV/PROD 回退
- [x] 修正 `docs/evidence/AEP-206/diffstat.txt` 插入数为 391
- [x] 136/136 CTest -j1 + 10/10 Omnis + Go 测试全绿
- [x] 全套反熵门禁通过
- [x] 真实证据日志入库
