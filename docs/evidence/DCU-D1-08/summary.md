# DCU-D1-08 — NormalList contract

- Base: latest main (`c497b711`, includes D1-05 merge `ac7b8dfe`)
- Branch: `dcu/D1-08-normal-list`
- Date: 2026-11-22

## Five required cases

1. **All items succeed** — kept: `test_driver_normal_list_uses_axdr_per_item`
   (2 OADs, per-item A-XDR bytes).
2. **Mixed success / failure** — `test_driver_normal_list_mixed_success_and_dar`.
   Request `[2000, 2001, 2004]`; source returns NOT_FOUND for 2001. The whole APDU is
   compared byte for byte:
   ```
   85 02 01 03
   20000200 01 17 4367B333          Data
   20010200 00 06                    DAR 6, item still present and counted
   20040200 01 17 00000000          Data
   00 00
   ```
3. **Empty list (n=0)** — legal per the standard, so the slave answers an empty result
   list. `test_driver_normal_list_empty_request` and
   `test_dlt698_get_list::test_empty_list_is_legal` assert `85 02 01 00 00 00`.
   Basis: table 66/73 define `SEQUENCE OF OAD` / `SEQUENCE OF A-ResultNormal` with no
   `SIZE` lower bound (`docs/698_parsed.md:2575-2582`, `:2696-2707`), and the standard
   states a `SEQUENCE OF` with 0 data items is valid (`docs/698_parsed.md:1880`).
4. **Duplicate OADs** — `test_driver_normal_list_duplicate_oad_each_answered`: the same
   OAD twice is resolved twice; each item carries its own value (`00 01`, `00 02`), both
   present and counted.
5. **Acceptance per-item DAR** — `test_unimplemented_oad_gets_per_item_dar` sends
   `[20000200, 99990000]` through the real 698 slave. The response keeps the voltage
   reading (231.7 V, float32) and returns `[0] DAR 6` for the unknown OI. The point table
   cannot express "master asks for an object that does not exist", so the case sends a raw
   frame; the existing 6-OAD all-success acceptance case is unchanged.

## Defect found and fixed

The codec and driver treated `n == 0` as a bad frame:

- `dlt698_encode_get_response_list_begin` / `dlt698_encode_get_response_list`
  rejected `n == 0`;
- the driver rejected it before encoding.

The standard has no `SIZE` lower bound, so an empty NormalList is legal and must receive
an empty result list. Both encoders now accept `n == 0` (`items` may be NULL), the driver
forwards it, and `test_rejects_bad_arguments` no longer asserts the old rejection.

## Behaviour changes

- Empty NormalList requests (previously an unanswered/bad frame) now get
  `85 02 PIID 00 00 00`.
- Public contract corrected: `dlt698_codec.h` now documents `n ∈ [0, MAX]`.

## Open, outside this repo

- `testplatform.protocol.dlt698.parse_get_response_list` raises on per-item DAR and
  rejects `count == 0`, so the acceptance test parses the raw APDU. Extending the pinned
  platform parser would remove that duplication.
- The same module's `get_request_list` still documents/refuses `n == 0` ("empty request is
  a bad frame"), which now contradicts the product. It needs the same update when the
  platform pin is next bumped.

## Gates

`tests.log`, `runtime.log`: clean build 0 new warnings, 30/30 dcu+dlt698 in Release and
ASAN, guard selftest, `./test.sh --omnis dcu` 2 + 8 passed, `git diff --check` clean.
