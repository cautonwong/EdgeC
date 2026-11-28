# DCU-FORMAL-06 五栏规格：SET/ACTION/CONNECT/RELEASE 响应编码 CBMC 5.12 形式化证明

## 1. 业务规则
为 `foundation/src/protocol/dlt698_codec.c` 中其余 4 个响应编码路径建立有界 CBMC 5.12 形式化验证 harness（`formal/cbmc/dlt698_encode_responses.c`）、负向变异脚本（`formal/cbmc/negative/dlt698_encode_responses_mutation.sh`），并在 `formal/manifest/dcu.tsv` 登记 3 条目（`proof`、`cover`、`function`）：
1. `dlt698_encode_set_response(sa, piid, oad, dar, out)`
2. `dlt698_encode_action_response(sa, piid, oi, method, mode, dar, data, dlen, out)`
3. `dlt698_encode_connect_response(sa, piid, params, out)`
4. `dlt698_encode_release_response(sa, piid, out)`
若证明过程暴露缺少 `_fits(...)` 尾空间守卫的真实越界写缺陷，在 `foundation/src/protocol/dlt698_codec.c` 中补齐 `_fits(...)` 守卫（参数非法或容量不足返回 `-1`），然后再完成证明。

## 2. 不变量
1. **无越界写与指针安全**：在 `--bounds-check --pointer-check --div-by-zero-check --signed-overflow-check --conversion-check` 下，所有对 `out->b` 的写入严格落在 `[out->b.__buf, out->b.__buf + cap)` 与固定存储 `storage[FORMAL_MAX_LEN]` 边界内。
2. **契约返回码**：参数非法（`NULL`、`method == 0`、`dlen > 0 && !data`）或容量不足时必须返回 `-1`；合法且容量充足时返回 `0`。
3. **CBMC 5.12 建模纪律**：不使用动态 VLA；使用尾部对齐定长缓冲 `storage[FORMAL_MAX_LEN]`；先约束局部变量再写入结构体字段；`unwind >= max_len + 1`。

## 3. 可观测输出
1. `proof` 模式（`main`）：全部分支 CBMC 属性 `0 failed`（`VERIFICATION SUCCESSFUL`）。
2. `cover` 模式：`__CPROVER_cover(...)` 达成 `SATISFIED`（至少一条完整响应编码返回 `0` 可达）。
3. `function` 模式（`responses_reject`）：显式验证 `NULL` 参数、`method == 0`、`dlen > 0 && !data` 及容量不足均返回 `-1`。
4. 负向变异脚本 `formal/cbmc/negative/dlt698_encode_responses_mutation.sh`：移除关键 `_fits` 守卫后 CBMC 在 60s 内捕获失败。

## 4. 验收判据
1. `python3 ci/scripts/formal_manifest.py --root .` 通过。
2. 本地直接运行 `cbmc` 执行 `proof`、`cover`、`function` 均在 60s 内通过。
3. 负向变异脚本返回 `0`。
4. 若修改了 `dlt698_codec.c`，`ctest --output-on-failure -R 'dcu|dlt698'` 全绿且 0 warning。

## 5. 非目标
- 不将 CBMC 负向脚本写入 `ci/mutation_guards.tsv`。
- 不修改 `modules/edge_core` 或受保护的 `foundation/include/` 头文件。
