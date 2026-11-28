# DCU-FORMAL-05 五栏规格：GetRecord 流式响应编码 CBMC 5.12 形式化证明

## 1. 业务规则
为 `foundation/src/protocol/dlt698_codec.c` 中的 GetRecord 流式响应编码路径建立有界 CBMC 5.12 形式化验证 harness（`formal/cbmc/dlt698_encode_get_record.c`）、负向变异脚本（`formal/cbmc/negative/dlt698_encode_get_record_mutation.sh`），并在 `formal/manifest/dcu.tsv` 登记 3 条目（`proof`、`cover`、`function`）：
1. `dlt698_encode_get_record_response_begin(stream, sa, piid, oad, rcsd, n, record_count, out)`
2. `dlt698_encode_get_record_row(out, row, row_len)`
3. `dlt698_encode_get_record_response_end(stream, out)`
4. `dlt698_encode_get_record_response_dar(sa, piid, oad, rcsd, n, dar, out)`

## 2. 不变量
1. **无越界写与指针安全**：在 `--bounds-check --pointer-check --div-by-zero-check --signed-overflow-check --conversion-check` 下，所有对 `out->b` 的写入严格落在 `[out->b.__buf, out->b.__buf + cap)` 与固定存储 `storage[FORMAL_MAX_LEN]` 边界内，不发生整数溢出或越界指针运算。
2. **契约返回码**：所有公开函数在合法/非法输入下仅返回 `0` 或 `-1`；参数非法（`NULL` 指针、`n > 0 && !rcsd`、`record_count > 0xFF`、`row_len == 0`）或 tailroom 不足时必须返回 `-1`，不得静默截断或越界。
3. **CBMC 5.12 建模纪律**：不使用动态 VLA；使用尾部对齐定长缓冲 `storage[FORMAL_MAX_LEN]`（`out->b.__buf = storage + (FORMAL_MAX_LEN - cap)`）使 `tailroom == cap` 且越界可被精确捕获；先约束局部变量再写入结构体字段；`unwind >= max_len + 1`。

## 3. 可观测输出
1. `proof` 模式（`main`）：全部分支的 CBMC 属性 `0 failed`（`VERIFICATION SUCCESSFUL`），且包含 `begin -> row -> end` 流式成功路径与 `dar` 成功路径的长度/边界断言。
2. `cover` 模式：至少一条 `__CPROVER_cover(...)` 达成 `SATISFIED`（流式 `begin -> row -> end` 返回 0 的可达性防空洞守卫）。
3. `function` 模式（入口如 `get_record_reject`）：显式验证 `NULL` 参数、`n > 0 && !rcsd`、`record_count > 0xFF`、`row_len == 0`、以及容量不足均返回 `-1`（全部 `SUCCESS`）。
4. 负向变异脚本 `formal/cbmc/negative/dlt698_encode_get_record_mutation.sh`：移除 `dlt698_encode_get_record_row` 或 `dlt698_encode_get_record_response_begin` 中的关键容量守卫后，CBMC 必须在 60s 内捕获失败（非 0 且非超时 124）。

## 4. 验收判据
1. `python3 ci/scripts/formal_manifest.py --check --root .` 通过，且 `formal/manifest/dcu.tsv` 新增 3 行格式、字段与 `sync` SHA 完整有效。
2. 本地直接运行 `cbmc` 执行 `proof`、`cover`、`function` 均在 60s 内通过且无未展开断言失败（`--unwinding-assertions`）。
3. 运行 `formal/cbmc/negative/dlt698_encode_get_record_mutation.sh` 返回 0（即证明变异确实触发了预期失败）。
4. **严禁**将 CBMC 负向脚本写入 `ci/mutation_guards.tsv`（该文件由不含 CBMC 的 arch-gate 容器执行）。

## 5. 非目标
- 不修改 `foundation/src/protocol/dlt698_codec.c` 的生产代码（除非证明过程暴露真实越界缺陷，届时需另报或先修后证）。
- 不在本单验证 `dlt698_decode_data` 递归路径。
