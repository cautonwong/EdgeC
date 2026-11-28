# DCU-FORMAL-03 Independent Verification (`verify.md`)

- **Verdict**: `VERIFIED`
- **Merged Commit on `feat/dcu`**: `dcu/FORMAL-03-dlt698-decode-data-cbmc` (`aa4921aa`, verified on combined Batch 4 tip)
- **Spec Reference**: [spec.md](file:///home/EdgeC/docs/evidence/DCU-FORMAL-03/spec.md)

## Independent Expected Value Derivation (from Spec & Protocol)

From `spec.md` §1–§2 and `foundation/src/protocol/dlt698_codec.c:488`:
- `dlt698_encode_report_notification` writes `CHOICE(1) | PIID(1) | count(1)` (`3` bytes header) plus `4 (OAD) + 1 (Get-Result choice) + (dar ? 1 : dlen)` bytes per item.
- For `n = 1`, `dar = 0`, `dlen = 2`: `need = 3 + 4 + 1 + 2 = 10` bytes, so `cap = 10` must succeed with `*len = 10` (`<= cap`), and `cap = 9` must return `-1` without writing past `buf[9]`.
- `choice = DLT698_REPORT_RECORD_LIST (0x02)` must return `DLT698_REPORT_UNSUPPORTED_RECORD (-4)`.
- Under CBMC `--conversion-check`, masking `(uint8_t)((items[i].oad >> 16) & 0xFFu)`, `(uint8_t)((items[i].oad >> 8) & 0xFFu)`, and `(uint8_t)(items[i].oad & 0xFFu)` eliminates unsigned-to-unsigned truncation warnings while preserving identical wire bytes.
- Removing `if (need > cap) return -1;` must allow `buf[off++]` to overflow `out_buf[12]`, causing `dlt698_encode_report_mutation.sh` to catch a CBMC pointer-dereference FAILURE.

## Merged-Tree Verification Evidence

1. **Manifest Integrity**: `python3 ci/scripts/formal_manifest.py --root . --rows` → 13 rows parsed cleanly (`9 dcu`, `4 iems`).
2. **CBMC `proof` (`-DFORMAL_MAX_LEN=12 --unwind 13 --unwinding-assertions`)**: `0 of 2728 failed`, `VERIFICATION SUCCESSFUL`.
3. **CBMC `cover` (`-DFORMAL_MAX_LEN=12 --cover cover --unwind 13`)**: `[main.coverage.1] ...: SATISFIED`.
4. **CBMC `function` (`--function report_reject`)**: all assertions `SUCCESS`.
5. **Negative Mutation Check**: `formal/cbmc/negative/dlt698_encode_report_mutation.sh` → `FORMAL03_NEG_OK` (exit 0; catches out-of-bounds write FAILURE).
6. **Build, Repo Guards & Full CTest**: `check_build_warnings.py` 0 warnings; `run_guard_selftest.py` PASS; `check_doc_references.py --root .` PASS; `ctest` `381/381 passed`.
