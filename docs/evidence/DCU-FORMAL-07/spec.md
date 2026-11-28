# DCU-FORMAL-07 五栏规格：迭代版 `dlt698_decode_data` CBMC 5.12 形式化证明

## 1. 业务规则
为 `DCU-REFACTOR-01` 重构后的迭代版 `dlt698_decode_data(buf, len, offset, res)` 建立有界 CBMC 5.12 形式化验证 harness（`formal/cbmc/dlt698_decode_data.c`）、负向变异脚本（`formal/cbmc/negative/dlt698_decode_data_mutation.sh`），并在 `formal/manifest/dcu.tsv` 登记 3 条目（`proof`、`cover`、`function`）。

## 2. 不变量
1. **输入与栈/池边界安全**：在 `--bounds-check --pointer-check --div-by-zero-check --signed-overflow-check --conversion-check` 下，对任意符号化输入 `buf[0..FORMAL_MAX_LEN-1]`、`len <= FORMAL_MAX_LEN`、`*offset <= len`，不发生 `buf` 越界读、`stack[DLT698_DECODE_MAX_DEPTH]` 越界读写或 `g_dlt698_item_pool[64]` 越界写。
2. **契约返回码**：返回值严格属于 `{0, -1, -2}`；`NULL` 参数、截断输入、嵌套深度超过 `DLT698_DECODE_MAX_DEPTH`、池溢出均返回 `-1`；不支持 tag 返回 `-2`。
3. **CBMC 5.12 建模纪律**：不使用动态 VLA；使用尾部对齐定长缓冲 `storage[FORMAL_MAX_LEN]`；约束局部标量后再传入函数；`unwind` 足够覆盖 `FORMAL_MAX_LEN` 与 `DLT698_DECODE_MAX_DEPTH`。

## 3. 可观测输出
1. `proof` 模式（`main`）：全部分支 CBMC 属性 `0 failed`（`VERIFICATION SUCCESSFUL`）。
2. `cover` 模式：`__CPROVER_cover(rc == 0)` 达成 `SATISFIED`（且验证嵌套容器成功路径可达）。
3. `function` 模式（`decode_data_reject`）：显式验证 `NULL` 参数、截断输入、超深嵌套（9 层）、以及不支持 tag 的返回码。
4. 负向变异脚本 `formal/cbmc/negative/dlt698_decode_data_mutation.sh`：移除 `depth >= DLT698_DECODE_MAX_DEPTH` 守卫后 CBMC 在 60s 内捕获失败。

## 4. 验收判据
1. `python3 ci/scripts/formal_manifest.py --root .` 通过。
2. 本地直接运行 `cbmc` 执行 `proof`、`cover`、`function` 均在 60s 内通过。
3. 负向变异脚本返回 `0`。

## 5. 非目标
- 不修改 `foundation/src/protocol/dlt698_codec.c`（由 `DCU-REFACTOR-01` 完成；若发现真实缺陷则停止并报告 `@coordinator`）。
- 不将 CBMC 负向脚本写入 `ci/mutation_guards.tsv`。
