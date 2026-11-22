# DCU-D1-03 comment review

Tool: `pstack:comment-sicko` (no-comments), model opus, scope `81730385^1..81730385`.

## Result

- Files edited: 5. Comment-only. `git diff --stat` reports 8 insertions, 100 deletions.
- No code line changed. `git diff --check` exits 0.
- `HOLD`: none.

## Deleted

- `dcu_698_object_model.c`: file-header migration history, `/* 先声明后定义 */`, 11 section banners, 9 numbered object-table labels, 5 `[PRD Alignment]` method labels, 27 attribute/method/emoji narration tails, 2 inline `属性/方法未定义` tails.
- `dcu_698_object_model.h`: stale `dcu_object_t` migration paragraph, mask field restatements, old-registration sentence, `（旧 dcu_router_destroy 语义）`.
- `dcu_router.c`: D1-03 history paragraph.
- `dcu_router.h`: relocation marker for deleted code.
- `tests/test_dcu_698_object_model.c`: 5 test-body narration comments and the `D1-03` ticket prefix.

The deleted method and attribute labels restated the standard numbering already recorded in
`docs/design/DCU_698_OBJECT_MODEL.md` and `docs/design/DCU_698_OI_CATALOG.md`. The catalog is the
source of names. The table keeps the numeric masks.

## Kept

- `BIT`/`BYTE` macro rationale.
- No-lock concurrency invariant.
- `-4` refusal rationale for writes and Class 9 methods (no fake success).
- `revision++` poller state-retirement dependency.
- Legacy 127/128 compatibility used by `test_dcu_router`.
- NORMAL-only legacy scheme refusal.
- DataHub physical-truth source-of-truth note.
- Class 9 D1-15 placeholder rationale.
- Action value mapping `1=跳闸 0=合闸允许`.
- Aggregate-root / southbound control constraint.
- Event reverse-order semantic.
- Empty Class 9 branch placeholder.
- Public API contracts in `dcu_698_object_model.h` and `dcu_router.c` orientation.

## Verification after the cleanup

- `git diff --check` exit 0.
- `cmake --build build -j$(nproc)` exit 0.
- `ctest -R 'dcu|dlt698'` 26/26 passed.
- `run_guard_selftest.py --root .` all guards and live checks passed.
- `./test.sh --omnis dcu` dcu-acceptance 2 passed, dcu-northbound 4 passed.
