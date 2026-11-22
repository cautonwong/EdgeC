# DCU-D3-04a comment audit (`/no-comments`)

Reviewer: `pstack:comment-sicko` (opus), two passes: the rebased branch with the
local-civil-time and A6-invalid-stamp decisions, then the post-D1-20a rebase.

Second pass deletions (10) and trims:
- `cmd/dcu/dcu_698_record.c`, `cmd/dcu/dcu_event_monitor.c`: dropped the single-source
  architecture note and the snapshot-before-read rationale; both restated bodies or named
  where code lives.
- `tests/test_dcu_698_axdr.c`: dropped four assertion-narrating comments (A6 scalar write
  twice, the A6/A14 time rule summary, and the monotonic-as-epoch note). The 表194 A2/A3
  reference stays; its A5 status clause moved out.
- `tests/test_dcu_advanced_modules.c`: dropped three assertion-narrating comments; kept the
  legacy `event_history` unit constraint and trimmed its history sentence. Replaced the
  `a6 + 9` magic offset with named header sizes (`A6_ARRAY_HDR + A6_STRUCT_HDR +
  A6_INDEX_BYTES`), so the layout prose could die.
- `cmd/dcu/dcu_698_object_internal.h`: the `unit_count` method list now includes 133, which
  the decoder does set (the old list was wrong).

Kept because they state a rule the code cannot show: the 6.1.1.5 TimeTag rule, 表48/49 and
193/194 layouts, the local-civil TZ operational requirement (product ruling, repeated in
the axdr header and the three TZ test fixtures), the monotonic-versus-wall-clock split, and
the A5 masking reason.

No suppressions, commented-out code, or narrative banners remain in the diff.
