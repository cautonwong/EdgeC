# DCU-D1-08 — comment audit

`pstack:comment-sicko` reviewed the diff. No comment deletions, no `HOLD`s.

## Added comments (all kept)

- `tests/test_dcu_698_axdr.c` — table 73 has no `SEQUENCE OF` size floor; cites
  `docs/698_parsed.md:1880` for the 0-item rule.
- `tests/test_dlt698_get_list.c` — same rule next to the empty-list encoder test.
- `acceptance/dcu/tests/test_dcu_northbound.py` — DAR/Get-Result semantics and why the
  bench point table cannot express a nonexistent OAD (raw frame required).

## Workaround / duplication findings

1. **Stale public contract (fixed).** `dlt698_codec.h` documented `n ∈ [1, MAX]` while the
   encoder now accepts 0. Changed to `n ∈ [0, MAX]` with the NULL-items note.
2. **Hand-rolled APDU parsing in the acceptance test (open).** The platform parser
   `parse_get_response_list` raises on any DAR item and rejects count 0, so it cannot
   represent the mixed response this task proves. Fix belongs in the external pinned
   `test-platform` repo; the alternative would be hiding the per-item DAR behind a parser
   that refuses to expose it. Reported in `summary.md`.
3. **Platform drift (open).** `get_request_list` in the same external module still says an
   empty request is a bad frame. Needs the platform update; not editable from this repo.
4. Cosmetic: `assert_apdu` repeats the frame envelope as `9 + n + 3`; test fixtures repeat
   the 231.7 V bytes. Test-only, no production constant duplicated; left as is.
