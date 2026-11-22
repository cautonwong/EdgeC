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

## verifier 复审计

- `var_element_t.name` 注释改为真话：只有 attr2 元素有北向名，attr4 不参与命名。
  原文"NULL 表示不上报"会让下一个贡献者以为 NULL 能抑制上报，实际北向会回落成 `tag_<id>`。
- 删除 `_dcu_north_name` 的转发说明与派生测试头注释（都在复述代码/测试名）。
- MUST KILL：测试里 `obj->attr_mask[0] & 0x04` 改为 `dcu_mask_test(obj->attr_mask, 2)`，
  不再需要"属性 2 已广告"的旁白；补 `dcu_698_object_model.h`。
- `dcu_698_var.h` 的 `metric_name` 只保留 `@brief` 合同，删掉实现理由段。
- 保留：IEMS `edge_tag_meta_t.name` 命名风格约束、两条公开 API 合同、0x04 相邻的协议说明（未改行）。
