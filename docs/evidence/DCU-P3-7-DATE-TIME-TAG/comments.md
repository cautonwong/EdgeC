# 注释变更（DCU-P3-7-DATE-TIME-TAG）

行为不变的命名收口。只改既有注释使其与新增常量一致，未新增注释。

## 1. `foundation/src/protocol/dlt698_codec.c:541`

改动前：

```c
/* 区间值只接受 date_time（0x19）与 date_time_s（0x1C）。 */
```

改动后：

```c
/* 区间值只接受 DLT698_TAG_DATE_TIME 与 DLT698_TAG_DATE_TIME_S。 */
```

理由：常量就位后，注释按符号名表述，不再依赖裸数值。

## 2. `foundation/src/protocol/dlt698_codec.c:545`

改动前：

```c
    if (p[0] == 0x19) { /* date_time：无 DLT698_TAG_* 常量 */
```

改动后：

```c
    if (p[0] == DLT698_TAG_DATE_TIME) {
```

理由：行内注释“无 DLT698_TAG_* 常量”已不成立，随常量替换一并删除；`DLT698_TAG_DATE_TIME` 自解释，不另加注释。
