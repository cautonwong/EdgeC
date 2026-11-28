# DCU-FORMAL-03 Finding & Summary

## 1. `dlt698_decode_data` CBMC Tractability Finding

During initial formal verification of `dlt698_decode_data` (`foundation/src/protocol/dlt698_codec.c`), CBMC 5.12 hit an intractable `O(N^N)` unwinding and SSA blowup without modifying production code:

- `tag_reject` (`--function tag_reject`, concrete inputs): PASS in 1.37s (`0 of 2725 failed`).
- Symbolic `proof` (`--bounds-check --pointer-check --div-by-zero-check --signed-overflow-check --conversion-check`):
  - `FORMAL_MAX_LEN=12 --unwind 13` (negative mutation): timeout >60s (`EXIT=124`).
  - `FORMAL_MAX_LEN=8 --unwind 9`: timeout >30s (`EXIT=124`).
  - `FORMAL_MAX_LEN=6 --unwind 7`: timeout >60s (`EXIT=124`).
  - `FORMAL_MAX_LEN=5 --unwind 6`: timeout >60s (`EXIT=124`).
  - `FORMAL_MAX_LEN=4 --unwind 5`: timeout >30s (`EXIT=124`); `--cover cover` also timeout >15s (`EXIT=124`).
  - `FORMAL_MAX_LEN=3 --unwind 4`: timeout >30s (`EXIT=124`).
  - `FORMAL_MAX_LEN=2 --unwind 3`: timeout >30s (`EXIT=124`).
  - `FORMAL_MAX_LEN=1 --unwind 2`: timeout >15s (`EXIT=124`, 79,878 steps, stalled in `converting SSA`).

**Root cause**: `dlt698_decode_data` is recursive (`DLT698_TAG_ARRAY` / `DLT698_TAG_STRUCTURE` loop calling `dlt698_decode_data` on `&g_dlt698_item_pool[g_dlt698_pool_idx + i]`), where `g_dlt698_item_pool` is a 64-element `_Thread_local` array of `dlt698_data_t` (each containing `str.buf[64]` written via `memcpy`). Under global `--unwind N`, CBMC unrolls `O(N^N)` recursive copies and bit-blasts 64-way symbolic pointer writes at every copy.

**Resolution**: Per coordinator decision, `DCU-FORMAL-03` pivoted to `dlt698_encode_report_notification` (`foundation/src/protocol/dlt698_codec.c`), a non-recursive raw-buffer encoder.
