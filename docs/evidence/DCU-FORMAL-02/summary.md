# DCU-FORMAL-02 执行摘要

## 变更文件
- `docs/evidence/DCU-FORMAL-02/spec.md`
- `docs/evidence/DCU-FORMAL-02/summary.md`
- `formal/cbmc/dlt698_get_record.c`
- `formal/cbmc/negative/dlt698_get_record_mutation.sh`
- `formal/manifest/dcu.tsv`

## 关键设计说明
- 返回码契约：`dlt698_decode_get_record_request` 头文件契约（`foundation/include/dlt698_codec.h`）包含 `0`、`-1`、`DLT698_RECORD_UNSUPPORTED_SELECTOR`（`-2`）与 `DLT698_RECORD_UNSUPPORTED_DATA`（`-3`），`main` 断言覆盖全部 4 个合法返回码。
- 求解边界与缓冲区构造：`dlt698_decode_get_record_request` 包含嵌套循环（`i < count` 与 `j < csd->related_count`），且 `ci/scripts/formal_manifest.py` 要求 `unwind >= max_len + 1`。采用 `FORMAL_MAX_LEN=12`、`--unwind 13` 与尾部对齐固定缓冲 `apdu = buf + (FORMAL_MAX_LEN - len)`，既避免 VLA 导致 CBMC 5.12 SAT 子句爆炸，又使 `apdu[len]` 严格落于 `buf` 之外，确保越界读可被精确捕获，同时将 `proof` 与负向变异时间控制在 21–24s。
- 负向变异：移除 `foundation/src/protocol/dlt698_codec.c` 临时副本中的 `if (len - off < (size_t)csd->related_count * 4) return -1;` 后，CBMC 在 `_read_be32` 稳定报 `pointer outside object bounds` `FAILURE`。

## 验证结果
- `python3 ci/scripts/formal_manifest.py --root . --rows`：PASS
- CBMC `proof`（`-DFORMAL_MAX_LEN=12 --unwind 13 --unwinding-assertions`）：`0 of 2734 failed`，`VERIFICATION SUCCESSFUL`（~21s）
- CBMC `cover`（`-DFORMAL_MAX_LEN=12 --cover cover --unwind 13`）：`[main.coverage.1] ...: SATISFIED`（~9.5s）
- CBMC `function`（`--function selector_reject`）：`0 of 2731 failed`，3 条断言全 `SUCCESS`（~0.8s）
- 负向变异 `formal/cbmc/negative/dlt698_get_record_mutation.sh`：PASS（捕获 `_read_be32.pointer_dereference.*: FAILURE`，~24s）
- `python3 ci/scripts/check_doc_references.py --root .`：PASS
