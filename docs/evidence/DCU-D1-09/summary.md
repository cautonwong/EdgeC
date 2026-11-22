# DCU-D1-09 northbound metric names derive from the point model

Branch `dcu/D1-09-name-source`, base `dc1778e5` (D2-04 merge).

## Goal and change

`dcu_north_source.c` kept `s_dcu_north_names`, a second OAD-to-name table next
to the variable point model. The model already lists every shadow tag the
poller writes. The two could drift, and the name error is the hard kind: the
value is right and only the cloud field name is wrong.

The per-element metric names now live in `var_shape_t.names` in
`cmd/dcu/dcu_698_var.c`, next to the `shadow[]` tags they name. The north port
forwards to `dcu_698_var_metric_name` / `dcu_698_var_metric_resolve`; the
parallel table is deleted, together with its `dcu_router.h` dependency. OAD
constants stay in `dcu_router.h` as the only definition site.

Behavior for every point the poll scheme can produce is unchanged: the same 14
names, the same reverse mapping. Names are now impossible to desynchronize from
the tags because both sit in the same struct literal.

## Two names removed

`ZERO_SEQ_CURRENT` and `REACTIVE_POWER_TOTAL` were the only entries with no row
in the variable point model and no row in the object table. Neither is in
`dcu_698_var_poll_tags()`, so the default scheme never polls them and no value
can reach the shadow under those tags. Keeping them would break the parity
rule this ticket establishes. They now return NULL, as any unknown tag does.

Related defect found while surveying, not fixed here: `OAD_ZERO_SEQ_CURRENT` is
`0x20020200`, but DL/T 698 OI 2002 attribute 2 is
`电压相角` (voltage phase angle) per `DCU_698_OI_CATALOG.md:243`. The driver
point table maps it under the zero-sequence name. No current object or scheme
row uses it, so this is latent. It needs its own defect ticket; the name was
not carried into the derived table.

## Parity test

`tests/test_dcu_north_source.c::test_names_derive_from_point_model` iterates
`dcu_698_var_poll_tags()` and asserts, per tag:

1. `name()` returns non-NULL;
2. `resolve_name(name)` returns the same tag;
3. the tag's OI has an object row advertising attribute 2;
4. names are unique across tags.

Adding a point to `k_vars` without a name, or removing its object row, fails
the test. `test_names_exclude_points_absent_from_object_table` locks the other
direction for the two removed names.

## Gates

| Gate | Result |
| --- | --- |
| Clean build, `check_build_warnings.py` | pass, 0 new warnings |
| `ctest -R 'dcu\|dlt698'` | 39/39 pass |
| Full `ctest` | 342/342 pass |
| ASAN, `test_dcu_north_source` + `test_dcu_698_var` | pass |
| UBSAN, same targets | pass |
| `run_guard_selftest.py` | all guards live-pass |
| `run_fanalyzer.py` | 3 known, no new |
| `git diff --check` | clean |
| `./test.sh --omnis dcu` | 2/2 tasks, 21 northbound cases |

Comment audit and the final diffstat are in this directory.
