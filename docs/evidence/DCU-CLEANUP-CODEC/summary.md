# DCU-CLEANUP-CODEC：codec 魔数标签改用 DLT698_TAG_*

分支 `dcu/cleanup-codec-tags`，工作树 `/tmp/edgec-land/codec-tags`，基线 `feat/dcu`
`8236eb3d`。§12.5 的“codec 魔数标签”缺口：`foundation/src/protocol/dlt698_codec.c`
里的 A-XDR 类型标签用数字字面量写死。本单只把有对应常量的字面量换成
`DLT698_TAG_*`，不改任何编码字节。

## 替换

| 位置 | 原字面量 | 常量 | 值 |
| --- | --- | --- | --- |
| `_record_date_data` | `0x1C` | `DLT698_TAG_DATE_TIME_S` | 28 |
| `_record_interval_data` | `0x00` | `DLT698_TAG_NULL` | 0 |
| `_record_interval_data` | `0x54` | `DLT698_TAG_TI` | 0x54 |
| `dlt698_encode_long_unsigned` | `18` | `DLT698_TAG_LONG_UNSIGNED` | 18 |
| `dlt698_decode_long_unsigned` | `18` | 同上 | 18 |
| `dlt698_encode_double_long` | `5` | `DLT698_TAG_DOUBLE_LONG` | 5 |
| `dlt698_decode_double_long` | `5` | 同上 | 5 |
| `dlt698_encode_date_time_s` | `28` | `DLT698_TAG_DATE_TIME_S` | 28 |
| `dlt698_decode_date_time_s` | `28` | 同上 | 28 |

未替换：`_record_date_data` 的 `0x19`（date_time，25）在
`foundation/include/dlt698_codec.h` 里没有对应常量；原位注释
“date_time：无 DLT698_TAG_* 常量”。补常量前保持字面量。

## 等价性证据

- **级别 1（编译产物）**：替换前后
  `build/foundation/CMakeFiles/edge_codec.dir/src/protocol/dlt698_codec.c.o` 的
  sha256 均为 `eaa6b7c4bd1ba9c0f1119b02d0271306d24dfe531ba4938cf9c50dda39894dfc`；
  `objdump -d` 仅差对象文件名的表头行。
- **级别 2（行为测试）**：`ctest -R 'dlt698|driver_dlt698'` 替换前 7/7、替换后 7/7。
  H.1 帧向量在 `test_driver_dlt698`（29 例全过），H.3.3 逐字节向量在
  `test_dlt698_conformance_suite`（30 例全过）；测试文件未改。

## 守卫

- clean build `1267/1267`，第一方 warning 0；`check_build_warnings.py` 无新增。
- `check_build_freshness.py`：build 属于当前树且无待执行工作。
- `check_doc_references.py`：rc=0。

## 范围

只改 `foundation/src/protocol/dlt698_codec.c` 的 tag 字面量与 §12.5 行；不改长度、
CHOICE、控制码、DAR；无测试改动。未合并。
