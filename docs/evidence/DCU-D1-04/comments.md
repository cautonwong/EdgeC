# DCU-D1-04 comment review

Tool: `pstack:comment-sicko` (no-comments), model opus, scope `main...58ddf710`.

## Result

- Files edited: `cmd/dcu/dcu_698_var.c`, `tests/test_dcu_698_var.c`, `cmd/dcu/dcu_router.h`.
- Deleted 5 comments: the `D1-04` migration history in the file brief, the ticket prefix in the test brief, and three test-body assertions restated as comments.
- Kept the per-attribute A-XDR type tags, DataHub shadow/scale ownership, supported OI list, `-1` return-code rationale, the handler contract, and the standard OI/class mappings.
- Fixed a stale class label: `IC_VARIABLE 3` said `电能量`; Class 3 is 分相变量 (voltage, current, phase angle). Class 1 is 电能量.
- `HOLD`: none.

## Verification

- Comment-only except the stale class label text.
- `cmake --build build -j$(nproc)` after rebase.
- `ctest -R dcu`.
- `run_guard_selftest.py --root .`.
- `./test.sh --omnis dcu`.
