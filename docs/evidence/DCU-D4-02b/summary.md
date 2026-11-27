# DCU-D4-02b 验证总结

## 变更概述

- 分支：`dcu/D4-02b-class13-acl`（基于 `feat/dcu` @ `acef3050`）
- 在 `cmd/dcu/dcu_698_appconn.c` 的 `k_objects[]` 中以只读方式加入 `dcu_698_object_model.c:325-348` 已建模的 8 个 A.9 控制/参数 OI：
  - `0x8001`（保电）：A2..A5 只读（`rights=1`），0 方法
  - `0x8100`（终端保安定值）：A2 只读（`rights=1`），0 方法
  - `0x8101`（终端功控时段）：A2 只读（`rights=1`），0 方法
  - `0x8102`（功控告警时间）：A2 只读（`rights=1`），0 方法
  - `0x8103`（时段功控）：A2 只读（`rights=1`），0 方法
  - `0x810E`/`0x810F`/`0x8110`（控制对象/跳闸轮次/电控定值）：0 属性，0 方法（仅对象存在性）
- 对象列表总项数由 `118` 增至 `126`（`<= EDGE_SLAVE_ACL_MAX_OBJECTS = 224`），`4400 A2` 响应帧长 `6150 B`（可完整装入生产从站池 `BUS_SLAVE_POOL_BUF_SIZE`）。
- 同步更新 `docs/design/DCU_698_OBJECT_MODEL.md`，并在 `ci/mutation_guards.tsv` 新增变异守卫 `DCU-D4-02b-class13-acl`。

## 验证结果

1. Clean Ninja 构建 + `python3 ci/scripts/check_build_warnings.py --log build/build.log`：通过，0 新增编译告警。
2. `ctest --test-dir build -R '^(test_dcu_698_appconn|test_dcu_698_object_model|test_driver_dlt698|test_dcu_698_scheme_mgmt|test_dcu_698_axdr)$' --output-on-failure`：5/5 全部通过（见 `tests.log`）。
3. `python3 ci/scripts/run_guard_selftest.py`：Refutation 291/291 通过，全部 live 架构守卫通过。
4. `python3 ci/scripts/run_mutation_guards.py --build build --only DCU-D4-02b-class13-acl`：变异后测试失败（`rc=8`），恢复后测试通过（`rc=0`）。
5. `python3 ci/scripts/check_doc_references.py --root .`：通过。
6. `/no-comments`（`comment-sicko`）：仅 1 条测试内联注释被标出并已删除（见 `comments.md`）。
