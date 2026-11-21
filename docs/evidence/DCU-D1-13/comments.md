# DCU-D1-13 comment review

Tool: `pstack:comment-sicko` (no-comments), model opus, scope `main...3b2d1c9e`, code files only.

## Result

- Files edited: `foundation/include/edge_slave_plane.h`, `foundation/include/dlt698_codec.h`,
  `cmd/dcu/dcu_698_axdr.h`, `cmd/dcu/dcu_698_axdr.c`, `tests/test_dcu_698_axdr.c`.
- Deleted 10 comment units (23 lines): roadmap narration, three `dlt698_list_stream_t` field
  comments, two file banners, one test banner, one section banner, and two restatements.
- Kept the ADR-0013 contracts: `EDGE_SLAVE_AXDR_MAX`, `resolve_axdr`/`accept_axdr` NULL-only
  fallback, non-zero is a real failure, caller buffer, no `N × 2048`, TLS single-thread limit,
  buffer-too-small/no-truncation, 698 frame layout, and single-buffer reuse rationale.
- `HOLD` resolved: the archive-count comment said "标量对象", but `dcu_north_resolve_axdr` has an
  explicit `OI_METER_ARCHIVE` attr-2 branch. Reworded to "6000 attr 2 档案计数在对象层单独编码为
  long-unsigned".

## Accepted code flags

- The driver's bare DAR `6` / `7` were replaced with `DLT698_DAR_OBJECT_NOT_FOUND` and
  `DLT698_DAR_TYPE_MISMATCH`; the ACTION path uses `DLT698_DAR_SUCCESS` /
  `DLT698_DAR_TEMPORARY_FAILURE`. The constants live in `foundation/include/dlt698_codec.h`.
- The codec's magic tag literals `18` / `5` / `28` in the pre-existing encoders are outside this
  change. Recorded as a codec cleanup follow-up, not edited here.

## Verification

- `git diff --check`.
- `cmake --build build -j$(nproc)`.
- `ctest -R 'dcu|dlt698'`.
- `tests/test_dcu_698_axdr` 7/7.
- `run_guard_selftest.py --root .`.
- `./test.sh --omnis dcu`.
