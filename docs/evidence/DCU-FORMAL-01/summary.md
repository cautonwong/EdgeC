# DCU-FORMAL-01 Evidence Summary

## Goal
Bootstrap DCU's first CI-enforced CBMC formal verification shard (`formal/manifest/dcu.tsv`) for `dlt698_decode_frame_ex` in `foundation/src/protocol/dlt698_codec.c`, with bounded symbolic proof (`FORMAL_MAX_LEN=32`, `--unwind 33`), vacuity cover, concrete HCS/FCS rejection (`fcs_reject`), and negative guard mutation (`formal/cbmc/negative/dlt698_guard_mutation.sh`).

## Owned Files Changed
- `formal/manifest/dcu.tsv` — 3 manifest rows (`proof`, `cover`, `function`) with `owner=dcu`, `sync=acef3050`, `property_count=2740`.
- `formal/cbmc/dlt698_decode.c` — CBMC harness directly `#include`ing `../../foundation/src/protocol/dlt698_codec.c` (with `-DDLT698_CODEC_SOURCE` override support).
- `formal/cbmc/negative/dlt698_guard_mutation.sh` — mutation script removing `if (in->len < frame_len + 1) return 1;` from a temp copy of `dlt698_codec.c` and asserting CBMC catches an out-of-bounds failure in `dlt698_decode_frame_ex`.
- `docs/evidence/DCU-FORMAL-01/` — `spec.md`, `summary.md`, `tests.log`, `diffstat.txt`, `comments.md`.

## Verification Results
- `python3 ci/scripts/formal_manifest.py --root . --rows`: PASS (7 rows validated: 3 `dcu`, 4 `iems`).
- CBMC `proof` (`FORMAL_MAX_LEN=32`, `--unwind 33 --unwinding-assertions`): PASS (`0 of 2740 failed`, `VERIFICATION SUCCESSFUL`).
- CBMC `cover` (`FORMAL_MAX_LEN=32`, `--cover cover --unwind 33`): PASS (`[main.coverage.1] ... condition 'rc == 0': SATISFIED`, `1 of 1 covered`).
- CBMC `function fcs_reject`: PASS (`0 of 2739 failed`, `VERIFICATION SUCCESSFUL`, valid frame accepted + HCS/FCS bit-flip rejected).
- `formal/cbmc/negative/dlt698_guard_mutation.sh`: PASS (mutated copy failed with `15 of 2729 failed`, `VERIFICATION FAILED`, matched `[dlt698_decode_frame_ex.(pointer_dereference|array_bounds).*]: FAILURE`).
- `python3 ci/scripts/run_guard_selftest.py`: PASS (291/291 refutation tests passed, all live architecture guards passed).
- `python3 ci/scripts/check_doc_references.py --root .`: PASS (exit 0).
- `./ci/scripts/check_formal.sh --strict` & `./ci/scripts/check_formal_negative.sh --strict`: Base-environment blocker — `formal_tools.sh` pins `FORMAL_JAVA_MAJOR=17` for `iems.tsv`'s TLA+ row while this host has OpenJDK 21 (`/usr/lib/jvm/java-21-openjdk-amd64`), so both scripts exit 1 before running any CBMC rows.

## Findings
- No bugs found in `foundation/src/protocol/dlt698_codec.c`; production code was left untouched.
