# D1-09 comment audit

One pass with `pstack:comment-sicko` over the working-tree diff, base
`dc1778e5`. Findings and dispositions.

## Applied

1. Deleted the test comment above
   `test_names_exclude_points_absent_from_object_table`. It restated the
   asserts, and its "one-to-one" claim was false: the object table has many
   attribute-2 rows with no northbound name.
2. Trimmed the middle sentence of the comment above
   `test_names_derive_from_point_model`; kept the single-source reason and the
   guard purpose.
3. Structural (MUST KILL): replaced the parallel `shadow[4]` / `names[]` arrays
   in `var_shape_t` with `var_element_t { tag, name } elements[4]`. Element order
   mistakes are now unrepresentable in the type, and the ordering comment died
   with the shape.
4. Restored the one lost constraint from the deleted table: names follow IEMS
   `edge_tag_meta_t.name` style (uppercase snake) so cloud field names do not
   change per device.
5. Closed the residual risk from the audit: `test_name_maps_realtime_oads` now
   spot-checks `ACTIVE_POWER_A/B/C` and `REACTIVE_ENERGY`, since resolve-back
   cannot catch a name paired to the wrong tag.
6. Removed the now-unused `<string.h>` from `dcu_north_source.c`.

## Kept

`var_shape_t.elements` name comment (now only says NULL means not reported),
the two public API contracts in `dcu_698_var.h`, the single-source line in
`dcu_north_source.c`, the downlink protocol constraint block, and the `0x04`
mask explanation in the test.

`HOLD`: none.
