# DCU-D1-05 — OAD → typed value, one source

- Base: `2c88938b` (main)
- Branch: `dcu/D1-05-resolve-table`
- Date: 2026-11-22

## Change

`dcu_698_value_for_oad(oad, dcu_value_t*)` is now the only place that maps an OAD to an
A-XDR type tag. It tries the variable-class reader first, then the object-table scalar
descriptors. Scalar `resolve` and `resolve_axdr` both call it; the adapter no longer
carries OI chains or a tag→width switch.

Arrays/structures stay A-XDR-only: a scalar request for an attribute that exists but has
no scalar representation returns `EDGE_SLAVE_SRC_TYPE_MISMATCH` (DAR 7); an unregistered
OAD returns `EDGE_SLAVE_SRC_NOT_FOUND` (DAR 6).

## OAD → type

| OAD | Value | Type tag | width | Source |
| --- | --- | --- | --- | --- |
| 20000200 | A 相电压 | 0x17 float32 | 4 | variable class 3 |
| 20010200 | A 相电流 | 0x17 float32 | 4 | variable class 3 |
| 20040200 | 总有功功率 | 0x17 float32 | 4 | variable class 4 |
| 200A0200 | 总功率因数 | 0x11 long | 2 | variable class 4 |
| 200F0200 | 电网频率 | 0x12 long-unsigned | 2 | variable class 6 |
| 00100200 / 00100400 | 正向/反向有功电能 | 0x06 double-long-unsigned / 0x15 long64-unsigned | 4 / 8 | variable class 1 |
| 60000200 | 档案计数 | 0x12 long-unsigned | 2 | object table 6000 attr 2 |
| 60120200 | 任务方案数 | 0x12 long-unsigned | 2 | object table 6012 attr 2 |
| 50040200 | 日冻结记录数 | 0x12 long-unsigned | 2 | object table 5004 attr 2 |
| F200 attr 2 / attr 3 | 通道波特率 / 帧格式 | 0x06 double-long-unsigned / 0x13 unsigned | 4 / 1 | object table F200 |
| 3000/3005 attr 4,5,6,8,9 | 事件阈值/条数/使能 | 0x12 / 0x17 / 0x13 / 0x03 | 2/4/1/1 | object table |
| 3000/3005 attr 2 | 事件记录（数组/结构） | — | — | A-XDR only, scalar → DAR 7 |

The scalar port's `raw` is `int32_t` (`edge_slave_plane.h`), so the scalar callback rejects
8-byte types (`TYPE_MISMATCH`); `resolve_axdr` encodes them. This is covered by
`test_scalar_rejects_64bit_raw`.

## Bug found while making 5004 readable

`router_sink.ctx` is `&node->storage`, but `_node_count_freezes` cast it to `dcu_node_t *`
and took `&node->storage` again — a ~900 KB out-of-bounds pointer. Reading 5004 therefore
died in `uv_mutex_lock` (SIGABRT) inside the northbound thread. Fixed the cast in
`cmd/dcu/dcu_node.c`. Regression is end-to-end: acceptance case
`D_00_00_02_000007` reads 5004; without the fix the product exits during the read.

## Acceptance coverage

Six OADs in one `[2] GET-Request-NormalList`, matching the point table:

```
send    682700430001101d5405 02 01 06 20000200 20010200 20040200 60000200 60120200 50040200 00aa3f16
receive 20000200 00 01 17 4367b333   (231.7 V)
        20010200 00 01 17 00000000   (0.0 A, bench returns zero)
        20040200 00 01 17 00000000   (0.0 kW, bench returns zero)
        60000200 00 01 12 0028       (40 meters)
        60120200 00 01 12 0001       (1 task scheme)
        50040200 00 01 12 0000       (0 freeze records)
```

To make 2001/2004 readable, the poll scheme in `cmd/dcu/dcu_node.c` now polls three OADs
instead of only 2000. That file is outside the D1-05 file list but the acceptance
requirement cannot be met without it; the change is reviewed here.

## Gates

See `tests.log` and `runtime.log`: clean build (0 new warnings), 29/29 dcu+dlt698 tests in
Release and ASAN, guard selftest, `./test.sh --omnis dcu` (2 + 7 passed), `git diff --check`.
