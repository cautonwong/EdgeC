# DCU-FORMAL-05 独立验证报告

## 1. 独立预期推导
- **标准与契约依据**：`foundation/src/protocol/dlt698_codec.c` 的 `dlt698_encode_get_record_response_begin`、`dlt698_encode_get_record_row`、`dlt698_encode_get_record_response_end`、`dlt698_encode_get_record_response_dar`。
- **独立期望值**：
  - `dlt698_encode_get_record_response_begin` 帧前缀长度为 `8 + sa->len + 3 + 4 + 1 + _rcsd_len(rcsd, n) + 2`（当 `n=0, sa->len=1` 时为 `8 + 1 + 3 + 4 + 1 + 1 + 2 = 20` 字节），加上 `row_len`（至少 1 字节）与 `DLT698_GET_RECORD_TRAILER_LEN`（5 字节：2 字节可选字段 + 2 字节 FCS + 1 字节结束符 `0x16`），最小完整流式帧为 `26` 字节（因此 `FORMAL_MAX_LEN = 26` 可达 `rc_end == 0`）。
  - 证明过程中暴露的 2 处真实边界缺陷（`n > DLT698_RCSD_MAX` 未在 `_get_record_prefix_len` 前拦截导致 `rcsd[i]` 越界读；`row_len + DLT698_GET_RECORD_TRAILER_LEN` 存在 `size_t` 回绕风险）已按根因修复后再证。

## 2. 合并树实测结果（`feat/dcu`）
- **Manifest 校验**：`python3 ci/scripts/formal_manifest.py --root .` — `19 row(s) in 2 shard(s)` 通过。
- **CBMC `proof`**（`-DFORMAL_MAX_LEN=26 --unwind 27 --unwinding-assertions`）：**0 of 3403 failed**，`VERIFICATION SUCCESSFUL`。
- **CBMC `cover`**（`--cover cover --unwind 27`）：`[main.coverage.1] ... condition 'rc_end == 0': SATISFIED`（`1 of 1 covered`）。
- **CBMC `function`**（`--function get_record_reject`）：**0 of 3402 failed**，`VERIFICATION SUCCESSFUL`。
- **负向变异**：`formal/cbmc/negative/dlt698_encode_get_record_mutation.sh` — 退出码 `0`（成功捕获移除 `begin` tailroom 守卫导致的越界）。
- **全量回归**：`ctest --test-dir build --output-on-failure` — **382/382 passed**。
- **结论**：`VERIFIED`。
