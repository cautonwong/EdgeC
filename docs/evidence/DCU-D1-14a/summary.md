# DCU-D1-14a — 698 OAD parsing without signed-shift UB

- Base: main `e461bd96`
- Branch: `dcu/D1-14a-be32`
- Date: 2026-11-22

## Finding

UBSAN on the full suite reported:

```
adapters/io_drivers/src/driver_dlt698.c:269:32: runtime error:
left shift of 242 by 24 places cannot be represented in type 'int'
```

`payload[0] << 24` promotes `payload[0]` to `int`; for an OAD whose high byte is
0xF2 the shift overflows signed int (UB). The same expression appeared at five
sites (GET master decode, SET, NormalList, GET slave, SET slave).

## Fix

One helper in `driver_dlt698.c`:

```c
static uint32_t _read_be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}
```

All five sites now call it. No behaviour change for in-range bytes; the wire
order is unchanged.

## Guard

`test_driver_high_byte_oad_has_no_signed_shift` sends OAD `0xF2000200` through
both GET and SET and asserts the echoed OAD bytes and DAR, so the parser is
covered without depending on UBSAN.

A/B under UBSAN: the old expression produces the reported runtime error; the
helper produces zero. Both runs are in `runtime.log`.

## Gates

- Release: 30/30 `ctest -R 'dcu|dlt698'`.
- UBSAN (`build-ubsan`, `-DENABLE_UBSAN=ON`): 30/30, **0** `runtime error` lines;
  targeted test 24/24.
- Clean-build warning gate: 0 new.
- Guard selftest: pass.
- Acceptance `./test.sh --omnis dcu`: dcu-acceptance 3 passed, dcu-northbound 9
  passed.
- `git diff --check`: clean.

## Diff scope

Only `adapters/io_drivers/src/driver_dlt698.c` (helper + five call sites) and
the required test in `tests/test_dcu_698_axdr.c`. Nothing else touched, so the
D1-17 branch can rebase onto this.
