# DCU-FIX-578-3：6014/128 对存在且被引用的方案返回 DAR 6

## 结论

方法 128 把"方案不存在"和"方案被活动任务引用"合并成 `EDGE_SLAVE_SRC_NOT_FOUND`（DAR 6），而 GET 6014 A2 明明能读到该方案；同条件在 127/130 回 `DENIED`（DAR 3），129 回 `DENIED`。修复后两者分离：不存在回 DAR 6，存在但被引用回 DAR 3，并补 GET 仍可见的断言。

## 根因

`cmd/dcu/dcu_scheme_store.c` 方法 128 预检：

```c
if (!dcu_poller_get_scheme(poller, op->ids[i]) ||
    dcu_poller_scheme_referenced(poller, op->ids[i])) {
    return EDGE_SLAVE_SRC_NOT_FOUND;
}
```

## 修复

- 128 拆成两个判断：无此方案 → `EDGE_SLAVE_SRC_NOT_FOUND`；被引用 → `LOG_WARN` + `EDGE_SLAVE_SRC_DENIED`（与 127/130 的"拒绝改记录列选择 / 拒绝 Set_CSD"、129 的"拒绝 Clear"对齐）。
- 批量删除仍是全成全败：任一 id 被拒即整体不执行（既有语义，测试保持）。

## 完成定义

- 静态分析：`check_build_warnings.py` 通过（tests.log 末段，0 violations）。
- 测试：`tests/test_dcu_698_schemes.c`
  - `test_referenced_scheme_refuses_csd_change_and_delete`：删除被引用方案断言改为 `EDGE_SLAVE_SRC_DENIED`；新增"GET 6014 A2 仍服务方案 7"断言（经 `dcu_north_resolve_axdr` 的真实 GET 路径，`_a2_has_scheme_id` 校验）。
  - `test_id_zero_and_batch_atomicity`：同条件的批量删除期望从 NOT_FOUND 改为 DENIED；两方案仍存在，原子性不变。
  - 修复前：`-20 != -25`（line 345 与 line 540 两处），exit=2。
  - 修复后：12/12 通过；ASAN 构建 12/12 通过。
- 运行时：`harness_scheme_delete_dar`（adv-4）
  - 前：`6014 method 128 delete referenced scheme 7: rc=-20 (NOT_FOUND -> DAR 6)`，而 `GET 6014 sees scheme 7: yes`。
  - 后：`rc=-25 (DENIED -> DAR 3)`；127/129 仍 `-25`；GET 仍能读到方案。
- 证据文件：`tests.log`、`runtime.log`、`diffstat.txt`、`comments.md`。

## 未决项

- 无。128 的"不存在"路径仍回 DAR 6，由 `_delete_ids({9})` 的既有断言覆盖。

## 提交

- `3ab73242` test: 失败复现（先失败，含 line 345 与 line 540）
- `0a307adb` fix: 拆分 NOT_FOUND / DENIED
- `067684be` style: /no-comments 后改走真实 GET 路径并抽 `_a2_has_scheme_id`

分支 `dcu/fix-578-3-scheme-dar`，worktree `/tmp/edgec-land/fix3-scheme-dar`。未合并，等 dcu-verifier 验证。
