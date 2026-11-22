# DCU-D1-20a comment audit (`/no-comments`)

Reviewer: `pstack:comment-sicko` (opus), on the D1-20a diff.

2 flags applied, 0 deletions of legitimate why:
- `cmd/dcu/dcu_north_adapter.c`: trimmed the changelog tail "不再一律 NOT_FOUND" from the
  non-8000 routing comment; kept the boundary rationale.
- `tests/test_dcu_698_axdr.c`: deleted the "未知 OI ... DAR 6" comment; the test name and the
  asserted 6 already carry it.

Kept (non-obvious why): the routing-ACL fixture intent, the DAR-5 rationale (class-defined but
unexposed), the 5004/3 fixture note, the 6012 method-1 handler note, and the budget-ledger entry.

2026-11-22 rebase/extension: the final tree adds the 6012 time-tag why in
`cmd/dcu/dcu_698_axdr.c` and the fixture comments in `tests/test_dcu_698_axdr.c`
/ `tests/test_dcu_698_tasks.c`. They follow the same rule (only non-obvious why);
comment-sicko did not re-review them in this round.
