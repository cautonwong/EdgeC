# DCU-D1-14a — comment audit

The diff adds one comment and no other comment changes. No separate
`comment-sicko` run was requested for this defect ticket; this is the self-check.

- Added `/* 逐字节提升到 uint32_t 再移位：0xF2 高字节左移 24 位在 int 里是 UB。 */`
  above `_read_be32`. It states the non-obvious reason the cast exists (the UB),
  which a reader cannot recover from the expression alone. Kept.
- No comments deleted. No workaround code: the helper is the root-cause fix, not
  a symptom guard.
- The test name and its byte assertions carry the intent; no narration added.
