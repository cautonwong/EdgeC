# DCU-D1-20a comment audit (`/no-comments`)

Reviewer: `pstack:comment-sicko` (opus), two passes: the original D1-20a diff, then the
rebased tree with the time-tag amendment.

Deletions in the final pass (4):
- `cmd/dcu/dcu_698_axdr.c`: dropped the line restating the `_time_tag_absent` call and the
  Class 10 reset comment that repeated the 6.1.1.5 rule already on the helper.
- `tests/test_dcu_698_axdr.c`: dropped the DAR-5 test header and the reaches-handler header;
  both restated the test name and the asserted value.

Trims (4):
- `cmd/dcu/dcu_north_adapter.c`: kept only the A.7/A.2 spec attribution; the 8000 DAR-2
  constraint lives in the handler.
- `tests/test_dcu_698_axdr.c`: dropped changelog tails from the routing-ACL comment and the
  5004 method-3 note, and the format restatement above the 6012 A2 fixture.

Kept (non-obvious why): the 6.1.1.5 TimeTag rule and its 0x00 encoding on
`_time_tag_absent`; fixture annotations for opaque bytes (`0x05` means a time tag is
present); the routing-ACL allow-list; the composition-root budget ledger entry.
