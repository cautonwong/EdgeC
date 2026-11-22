# D1-16 comment audit (`/no-comments`)

Reviewer: `pstack:comment-sicko` (opus), on the staged diff.

Result: 13 narration comments deleted, 2 trimmed to keep only the non-obvious why.
No `do not remove` / constraint comments in this change, so nothing needed encoding.

Deleted:
- `driver_dlt698.c`: field restatements on `report_pending` / `report_piid`, the duplicate SA
  note at the assignment site, and the `blen == 0` narration.
- `dcu_698_report.c`: the `next` signature restatement.
- `tests/test_driver_dlt698.c`: the section banner and the assertion narrations.
- `tests/test_dcu_698_report.c`: `一条`, `队列空了`, `两条合在一帧`.

Trimmed:
- `dcu_event_monitor.c`: dropped the `💡 3.` marker and the first sentence; kept the ADR-0017
  pointer and the deferred `CHOICE[2]` scope note.
- `dcu_698_report.c` `_encode_scalar`: kept only "宽度未知（没有编码器）就丢弃这一条，不猜字节序".

Kept (legitimate non-obvious why):
- SA storage rationale, `session_manager_reset_sessions` reset note, 表99 PIID-mismatch rule,
  "no stacked unconfirmed reports", `CHOICE + PIID` minimum body, protocol byte layout and the
  bare-buffer overflow note, public port contract docs, threading / drop-oldest policy,
  attribute-4 meaning, and the protocol intent in the tests.
