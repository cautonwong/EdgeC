# DCU-D3-04a comment audit (`/no-comments`)

Reviewer: `pstack:comment-sicko`, on `git diff 2f502b50` (the rebased branch plus the
local-civil-time and A6-invalid-stamp decisions).

Applied:

- `cmd/dcu/dcu_698_class_event.c`: shortened `_put_required_stamp`'s comment to the why
  (`没有墙钟就拒绝，不把单调时标或 0 编成公历日期`), removing the clause that restated the
  `if`.
- `tests/test_dcu_698_record.c`: deleted the trailing `hour = 08` comment on `row[5]`; the
  test's own comment already establishes T1's local hour.
- `tests/test_dcu_advanced_modules.c`: deleted the comment above the three event-time
  assertions that narrated them.

HOLD, resolved by the verifier decision, kept:

- The repeated "date_time_s is device-local civil time, encode via localtime_r and decode
  via mktime; deployment must set TZ" comments in `dcu_698_axdr.c`, `dcu_698_axdr.h`,
  `dcu_698_record.c`, and the three test TZ fixtures. Table 49 itself is zone-free; the
  expectation of local civil time is a product/verifier ruling, not a standard quote. It is
  now the decided behavior, so the comments state the operational requirement and stay.

Kept because they encode a non-obvious why: Table 48/49 and 193/194 layouts, the
monotonic-versus-wall-clock split, the legacy `event_history` unit boundary, and the A5
masking reason.

No suppressions, commented-out code, or narrative banners remain in the diff.
