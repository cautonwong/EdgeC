# DCU-P3-12 证据包摘要：4000 A2 SET 失败 DAR 契约与实现对齐

- **分支**：`dcu/P3-12-4000-set-dar`（基于 `feat/dcu` `c0eaf332`）
- **Worktree**：`/tmp/dcu-P3-12`

## 根因与结论

- `cmd/dcu/dcu_698_param.c:432-444` 的实现本身符合 DL/T 698.45 语义：
  - 授权且合法 `date_time_s` + `g_clock->set` 成功 → `0`（DAR 0）
  - 授权且合法 `date_time_s` + 时钟端口缺失或 `g_clock->set` 失败 → `EDGE_SLAVE_SRC_TEMPORARY`（DAR 2）
  - 授权且 A-XDR 载荷畸形（错误 tag / 越界字段 / 尾随字节）→ `EDGE_SLAVE_SRC_TYPE_MISMATCH`（DAR 7）
  - 未授权会话由 `adapters/io_drivers/src/driver_dlt698.c` ACL 拦截 → `DLT698_DAR_DENIED`（DAR 3）
- 漂移来自验收条款 `acceptance/dcu/tests/test_dcu_northbound.py`（第 24 行判据与 `D_00_00_02_000022` 写 `DAR 2/3` 且未覆盖 DAR 7）及 `docs/design/DCU_698_OBJECT_MODEL.md` §8.1 遗留的“待修”描述。

## 改动清单

1. `tests/test_dcu_698_param.c`：在 `test_datetime_attrs_and_ranges` 中补充错误 tag（`0x11 0x01`）DAR 7 用例，并显式断言 DAR 7 后 `GET 4000 A2` 时间保持不变。
2. `tests/test_driver_dlt698.c`：新增 `test_slave_set_4000_a2_dar_contract`，经由 `driver_dlt698` 完整 APDU + ACL + `dcu_north_accept_axdr` 端到端覆盖 DAR 0 / 2 / 7 / 3 及每次操作后的 `GET 4000 A2` 回读。
3. `acceptance/dcu/tests/test_dcu_northbound.py`：同步顶部判据第 6 条与 `D_00_00_02_000022` 的 `clause` 与断言（合法 `date_time_s` 仅允许 DAR 0/2，移除 DAR 3，并补充畸形载荷回 DAR 7 且时间不变的检查）。
4. `docs/design/DCU_698_OBJECT_MODEL.md`：更新 §8.1 记录 `DCU-P3-12` 已对齐的 `0 / 2 / 7 / 3` 契约。
5. `ci/mutation_guards.tsv`：登记 `DCU-P3-12-4000-set-dar` 变异守卫，验证将 `EDGE_SLAVE_SRC_TEMPORARY` 改为 `EDGE_SLAVE_SRC_DENIED` 时 `test_slave_set_4000_a2_dar_contract` 必红、恢复后必绿。
6. `docs/evidence/DCU-P3-12/`：`spec.md`、`summary.md`、`tests.log`、`diffstat.txt`、`comments.md`。
