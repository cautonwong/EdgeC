# DCU-FORMAL-02 Independent Verification (`verify.md`)

- **Verdict**: `VERIFIED`
- **Merged Commit on `feat/dcu`**: `797f03a6`
- **Spec Reference**: [spec.md](file:///home/EdgeC/docs/evidence/DCU-FORMAL-02/spec.md)

## Independent Expected Value Derivation (from Spec & Protocol)

From `spec.md` §1–§2 and `foundation/include/dlt698_codec.h`:
- `dlt698_decode_get_record_request` parses `OAD(4) | RSD | RCSD [| 0x00]`.
- Minimum valid request (NO_SELECTION + 0 CSDs): `4 (OAD) + 1 (selector=0x00) + 1 (count=0) = 6` bytes.
- With tail-aligned fixed buffer `apdu = buf + (FORMAL_MAX_LEN - len)` (`FORMAL_MAX_LEN = 12`, `--unwind 13 --unwinding-assertions`), any read at `apdu[len]` strictly overflows `buf[12]`, so CBMC catches real out-of-bounds reads across all symbolic lengths `0..12` (including `len < 5`, `NO_SELECTION`, `OAD` CSDs, `ROAD` CSDs, truncated `SELECTOR1`/`SELECTOR2`, unsupported selectors/data tags, and trailing time-tag checks).
- Contract return codes are `{0, -1, DLT698_RECORD_UNSUPPORTED_SELECTOR (-2), DLT698_RECORD_UNSUPPORTED_DATA (-3)}`.
- On `rc == 0`: `consumed <= len`, `rcsd.count <= DLT698_RCSD_MAX (8)`, and every ROAD item satisfies `related_count <= DLT698_ROAD_MAX (4)`.
- Removing the `len - off < (size_t)csd->related_count * 4` guard must allow a symbolic ROAD entry to read past `buf[12]` in `_read_be32`, producing a CBMC pointer-dereference FAILURE.

## Merged-Tree Verification Evidence (`feat/dcu` @ `797f03a6`)

1. **Manifest Integrity**: `python3 ci/scripts/formal_manifest.py --root . --rows` → 10 rows parsed cleanly (`6 dcu`, `4 iems`).
2. **CBMC `proof` (`-DFORMAL_MAX_LEN=12 --unwind 13 --unwinding-assertions`)**: `0 of 2734 failed`, `VERIFICATION SUCCESSFUL`.
3. **CBMC `cover` (`-DFORMAL_MAX_LEN=12 --cover cover --unwind 13`)**: `[main.coverage.1] ...: SATISFIED`.
4. **CBMC `function` (`--function selector_reject`)**: concrete 24-byte Selector 1 + RCSD accepted (`rc == 0`), unsupported selector rejected (`-2`), truncated Selector 2 rejected (`-1`) — all `SUCCESS` (`0 of 2731 failed`).
5. **Negative Mutation Check**: `formal/cbmc/negative/dlt698_get_record_mutation.sh` → `FORMAL02_NEG_OK` (exit 0; mutated copy failed with `_read_be32` pointer dereference FAILURE).
6. **Repo Guards & Full CTest**: `run_guard_selftest.py` PASS; `check_doc_references.py --root .` PASS; `ctest` `380/380 passed`.
