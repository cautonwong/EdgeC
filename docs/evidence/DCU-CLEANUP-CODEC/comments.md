# DCU-CLEANUP-CODEC 注释自审

范围：refactor commit `69dc71bb` 的注释变化（未跑 comment-sicko；按仓库注释纪律
人工自评）。

## 新增

- `foundation/src/protocol/dlt698_codec.c` `_record_date_data` 的
  `/* date_time：无 DLT698_TAG_* 常量 */`：说明为什么同函数里的 `0x1C` 换成了常量
  而 `0x19` 保持字面量，避免下一个 agent 误以为漏改。

## 保留

- 三个 `dlt698_encode_*` 行尾注释里的 `Tag = 18/5/28` 保留：它们记录线缆上的数值，
  换成常量后仍是有效对照。
- 未删除任何既有注释。

## 删除

- 无。
