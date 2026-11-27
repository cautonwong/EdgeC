# DCU-P3-7-DATE-TIME-TAG：补 `DLT698_TAG_DATE_TIME` 并替换 codec 裸 `0x19`

状态：已完成。行为不变（同一 worktree、同一配置下目标对象 sha256 前后一致）。
基线 `feat/dcu` = `main` = `99318550`；分支 `dcu/P3-7-date-time-tag`。

## 1. 改动

- `foundation/include/dlt698_codec.h`：枚举 `dlt698_data_tag_t` 按数值顺序新增 `DLT698_TAG_DATE_TIME = 25`（位于 `FLOAT64 = 24` 与 `DATE_TIME_S = 28` 之间），未改动任何已有枚举值。
- `foundation/src/protocol/dlt698_codec.c`：
  - `_record_date_data` 的 `p[0] == 0x19` 改为 `p[0] == DLT698_TAG_DATE_TIME`；
  - 该函数上方注释与行内注释同步改为按常量名表述，删除“无 DLT698_TAG_* 常量”的过时说法（属于替换既有注释，未新增注释）。
- 未改 tests：现有用例已覆盖 date_time 11 字节分支，本改动不改变行为。

## 2. 行为不变的证据

见 `object-sha256.txt`：`edge_codec` 目标下 `dlt698_codec.c.o` 在改动前后 sha256 均为
`eaa6b7c4bd1ba9c0f1119b02d0271306d24dfe531ba4938cf9c50dda39894dfc`，逐字节一致。

## 3. rg 证据

见 `rg.txt`。改动后 `foundation/src/protocol/dlt698_codec.c` 中已无标签路径裸 `0x19`；仅剩的 `0x19` 子串命中：

- `dlt698_codec.c:20`：`DLT698_FCS_TAB`（FCS16 常量表）中的表值 `0x19c9`，不是标签，非标签路径。
- `modbus_codec.c:11`：Modbus CRC 常量表中的 `0x1980`，与 DLT698 标签无关。

说明：任务描述中写的是“SHA-256 常量表”，实际该表是 DLT698 FCS16 表；结论不变——剩余命中均为查表常量，不是标签字面量。

## 4. 测试

见 `tests.log`（Release，web 关闭，`-j1`）：

- `ctest --test-dir build -j1 -R 'dlt698' --output-on-failure`：7/7 通过。
- `ctest --test-dir build -j1 -R 'dcu|dlt698' --output-on-failure`：45/45 通过。

## 5. 未决

1. （已由 §7 处理）`cmd/dcu/dcu_698_axdr.c` 的 date_time 裸标签已改为常量；同类残留见 §7 的 rg 结果。
2. `docs/plans/2026-11-21-dcu-698-production-plan.md` 中“补 `DLT698_TAG_DATE_TIME` 后再替换 `0x19`”的条目（约 954 行）本次已落地，但按任务约束未改 docs/plans，留给计划维护者更新状态。

## 6. 附注

`diffstat.txt` 由 `git diff --stat 99318550..HEAD` 生成于证据文件已提交的版本，因此不包含 `diffstat.txt` 自身这一行。

## 7. 追加收口（verifier 复核要求）

- `cmd/dcu/dcu_698_axdr.c`：`dcu_698_date_time_s_to_epoch` 的 `data[0] == 0x19` / `data[0] == 0x1C`
  改为 `DLT698_TAG_DATE_TIME` / `DLT698_TAG_DATE_TIME_S`；同文件 `dcu_698_put_date_time_s`
  编码处的 `buf[(*off)++] = 0x1C` 一并改为常量（同一标签路径，行为不变）。
- 未动 `DLT698_FCS_TAB`（`dlt698_codec.c`）与 Modbus CRC 查表中的 `0x19` 子串。
- 第二轮（verifier 要求）：`dcu_698_class_event.c` `_put_invalid_stamp` 的 `0x1C` 改为
  `DLT698_TAG_DATE_TIME_S`（该文件显式 include `dlt698_codec.h`）；`dcu_698_axdr.h:76/84`
  注释里的裸 `0x19`/`0x1C` 改为常量名。
- rg：`rg -n "0x19|0x1C" cmd/dcu/ --glob "*.c" --glob "*.h"` 已无命中。
- 测试：`ctest --test-dir build -j1 -R 'dlt698|dcu' --output-on-failure` → 45/45 通过。
