# DCU-FORMAL-01 Independent Verification (`verify.md`)

- **Verdict**: `VERIFIED`
- **Merged Commit on `feat/dcu`**: `461a0ee0`
- **Spec Reference**: [spec.md](file:///home/EdgeC/docs/evidence/DCU-FORMAL-01/spec.md)

## Independent Expected Value Derivation (from Spec & Protocol)

From `spec.md` §1–§2 and DL/T 698.45 link-layer framing (`0x68 | L(2) | C(1) | SA_flag(1) | SA(1..16) | CA(1) | HCS(2) | APDU | FCS(2) | 0x16`):
- Minimum valid frame length with 1-byte SA and 2-byte APDU is `1 (0x68) + 2 (L) + 1 (C) + 1 (SA_flag) + 1 (SA) + 1 (CA) + 2 (HCS) + 2 (APDU) + 2 (FCS) + 1 (0x16) = 14` bytes (`frame_len = 13 + sa_len >= 14`, total buffer `>= frame_len + 1 = 15`).
- With `FORMAL_MAX_LEN = 32` and `--unwind 33 --unwinding-assertions`, CBMC explores all symbolic frames of length `0..32` (covering incomplete frames `< 12`, short `frame_len`, valid 1..16-byte SA frames, bad delimiters, bad HCS/FCS, and valid frames).
- When `dlt698_decode_frame_ex` returns `0`, `payload_out` must satisfy `buf <= payload_out < buf + len` and `(payload_out - buf) + payload_len < len - 2` (strictly before 2-byte FCS + `0x16`).
- Removing the `in->len < frame_len + 1` guard must allow a symbolic `l_field` larger than `in->len` to read `in->data[frame_len]` out of bounds, so `dlt698_guard_mutation.sh` must observe a real CBMC FAILURE.

## Merged-Tree Verification Evidence (`feat/dcu` @ `461a0ee0`)

1. **Manifest Integrity**: `python3 ci/scripts/formal_manifest.py --root . --rows` → 7 rows parsed cleanly (`3 dcu`, `4 iems`).
2. **CBMC `proof` (`--unwind 33 --unwinding-assertions`)**: `0 of 2740 failed`, `VERIFICATION SUCCESSFUL`.
3. **CBMC `cover` (`--cover cover --unwind 33`)**: `[main.coverage.1] ...: SATISFIED`.
4. **CBMC `function` (`--function fcs_reject`)**: valid frame accepted + corrupted HCS rejected + corrupted FCS rejected all `SUCCESS`.
5. **Negative Mutation Check**: `formal/cbmc/negative/dlt698_guard_mutation.sh` → `NEGATIVE_SCRIPT_OK` (exit 0; mutated copy failed with out-of-bounds FAILURE).
6. **Host Note**: `ci/scripts/check_formal.sh --strict` requires Java 17 (`FORMAL_JAVA_MAJOR=17`) for `iems.tsv`'s TLA+ row while this container has OpenJDK 21; CI installs Temurin 17 and runs all shards.
7. **Repo Guards & Full CTest**: `run_guard_selftest.py` PASS; `check_doc_references.py --root .` PASS; `ctest` `380/380 passed`.
