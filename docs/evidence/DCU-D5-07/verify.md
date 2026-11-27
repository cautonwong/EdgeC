# DCU-D5-07 Independent Verification (`verify.md`)

- **Verdict**: `VERIFIED`
- **Merged Commit on `feat/dcu`**: `9887fceb` (verified on combined Batch 3 tip `797f03a6`)
- **Spec Reference**: [spec.md](file:///home/EdgeC/docs/evidence/DCU-D5-07/spec.md)

## Independent Expected Value Derivation (from Spec)

From `spec.md` R1–R5:
- Every file path and symbol referenced in `cmd/dcu/CONTEXT.md`, `CONTEXT-MAP.md`, `docs/design/DOMAIN_CONTEXT_MAP.md`, and `docs/design/PUBLIC_STRUCT_AUDIT.md` must exist in the repository (`check_doc_references.py` exit 0).
- `docs/design/PUBLIC_STRUCT_AUDIT.md` must match the exact 10 public structs in `ci/baseline/public_structs.txt` (`check_public_structs.py` exit 0) and confirm 0 new public structs introduced by DCU 698.
- No hardware ESAM/DFU overclaims; `F100`/`F001` documented as honest DAR 5 without hardware/storage backing.

## Merged-Tree Verification Evidence (`feat/dcu` @ `797f03a6`)

1. **Doc Reference Gate**: `python3 ci/scripts/check_doc_references.py --root .` → PASS (exit 0).
2. **Guard Selftest & Live Architecture Guards**: `python3 ci/scripts/run_guard_selftest.py` → PASS (exit 0, includes `check_public_structs.py`).
3. **Full Test Suite**: `ctest --test-dir build --output-on-failure -j$(nproc)` → `380/380 passed`.
