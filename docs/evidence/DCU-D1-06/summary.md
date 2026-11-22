# DCU-D1-06 — northbound ACTION (OI 8000)

- Base: main `27d88ade`
- Branch: `dcu/D1-06-action`
- Date: 2026-11-22

## Method → outcome

| Method | A.9 param | Decode | Product result | DAR |
| --- | --- | --- | --- | --- |
| 127 触发告警 | NULL | OK | no alarm output | 2 |
| 128 解除报警 | NULL | OK | no alarm output | 2 |
| 129 跳闸 | `array<structure{OAD, unsigned, long-unsigned, bool}>` | OK (1..8 items) | no relay OAD → meter address mapping | 2 |
| 130 合闸 | `array<structure{OAD, enum}>` | OK (1..8 items) | no relay mapping | 2 |
| 131 电表明文合闸 | `array<structure{OAD, enum, visible-string}>` | OK (1..8 items) | no relay mapping / no password path | 2 |
| any 127…131 | wrong tag, wrong field count, 0 or >8 items, trailing junk | TYPE_MISMATCH | — | 7 |
| other methods | — | NOT_FOUND | — | 6 |
| unknown OI | — | NOT_FOUND | — | 6 |

`ACTION_MAX_ITEMS = 8` is the A.9 relay bound (attributes 4/5 are `bit-string(SIZE(8))`).
The decoder validates every item; only the first is retained (`dcu_action_param_t.count`
records how many were sent) until execution exists.

## Decode

`dcu_698_decode_action_param()` in `cmd/dcu/dcu_698_axdr.c` uses `dlt698_decode_data()`
(no hand-rolled tags, no dynamic memory — the codec's decoder is a TLS pool). It needed
two generic-decoder additions: the A-XDR OAD tag (81) and its 4-byte layout
(`foundation/src/protocol/dlt698_codec.c`, table 24 / 6.3.3.5 table 28). A trailing
time-tag byte after the Data field is allowed; anything longer is a shape error.

The driver previously collapsed every non-zero ACTION result to DAR 2, so DAR 7 was
unreachable. `_dar_for_action_rc()` now maps port codes through the shared
`_dar_for_source_rc()`, with DAR 2 as the ACTION default. Port contract updated in
`foundation/include/edge_slave_plane.h`.

## Blocker (reported, not faked)

There is no relay OAD → meter address (or DO channel) mapping anywhere in the product:
`dcu_inventory_by_slot()` is the only identity the control sink takes, and A.9's
"继电器 OAD" has no product-side interpretation. 129/130/131 therefore decode honestly
and answer DAR 2 rather than inventing a mapping. 127/128 have no alarm output; 131 has no
password-close path.

Legacy OI 8000 method 1 stays: the demo heartbeat (`dcu_heartbeat.c`) and
`dcu_router_action()` still reach it, and its tests pass. It is not reachable through the
northbound ACTION path.

## Tests

Unit (`tests/test_dcu_698_axdr.c`), one per method: legal shape → DAR 2; wrong shape →
DAR 7. Also exact decode fields for 129, a two-item 129 request, array-count 0 and >8,
non-structure item, bad OAD tag, missing/extra fields, wrong enum/string types, unknown
method → DAR 6, and trailing bytes → DAR 7.

Acceptance: 129 legal → DAR 2 then the same slave still answers GET 2000; 129 with a
missing bool field → DAR 7.

## Gates

`tests.log`, `runtime.log`: clean build 0 new warnings, 30/30 dcu+dlt698 in Release and
ASAN, guard selftest, `./test.sh --omnis dcu` 2 + 9 passed, `git diff --check` clean.
