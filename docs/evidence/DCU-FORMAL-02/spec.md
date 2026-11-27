# DCU-FORMAL-02 规格说明：`dlt698_decode_get_record_request` CBMC 形式化验证

## 1. 业务规则

- `dlt698_decode_get_record_request(apdu, len, oad, sel, rcsd, consumed)` 解码 GetRecord 请求载荷 `OAD(4) | RSD | RCSD`（允许末尾携带 1 字节 `DLT698_NO_TIME_TAG`）。
- 参数与最小长度判断：
  - `apdu`、`oad`、`sel`、`rcsd` 任一为 `NULL` 时返回 `-1`。
  - 函数入口清零 `*sel` 与 `*rcsd`，若 `consumed != NULL` 则先置 `*consumed = 0`。
  - `len < 5` 时返回 `-1`；否则按大端读取 4 字节 `*oad` 与 1 字节 `sel->selector`。
- RSD 选择器判断：
  - `DLT698_RSD_NO_SELECTION`（`0x00`）：无额外 RSD 字节。
  - `DLT698_RSD_SELECTOR1`（`0x01`）：读取 4 字节 `sel->oad` 与 1 个日期值（仅接受 `DLT698_TAG_DATE_TIME` 11 字节或 `DLT698_TAG_DATE_TIME_S` 8 字节）；截断返回 `-1`，不支持的数据标签返回 `DLT698_RECORD_UNSUPPORTED_DATA`（`-3`）。
  - `DLT698_RSD_SELECTOR2`（`0x02`）：读取 4 字节 `sel->oad`、起始日期、结束日期与间隔（仅接受 `DLT698_TAG_NULL` 1 字节或 `DLT698_TAG_TI` 4 字节）；截断返回 `-1`，不支持的数据标签返回 `DLT698_RECORD_UNSUPPORTED_DATA`（`-3`）。
  - 其余选择器值返回 `DLT698_RECORD_UNSUPPORTED_SELECTOR`（`-2`）。
- RCSD 列选择判断：
  - 读取 1 字节 `count`；若 `off >= len` 或 `count > DLT698_RCSD_MAX`（`8`）返回 `-1`。
  - 逐项解析 `count` 个 CSD：
    - `tag == 0x00`（`DLT698_CSD_OAD`）：需剩余至少 4 字节，读取 4 字节 `csd->oad`。
    - `tag == 0x01`（`DLT698_CSD_ROAD`）：需剩余至少 5 字节，读取 4 字节 `csd->oad` 与 1 字节 `csd->related_count`；若 `csd->related_count > DLT698_ROAD_MAX`（`4`）或剩余字节不足 `(size_t)csd->related_count * 4`，返回 `-1`；否则读取 `related_count` 个 4 字节关联 OAD。
    - 其余 `tag` 返回 `-1`。
- 尾部与成功返回（`0`）：
  - 若 `off < len`，仅当 `len - off == 1 && apdu[off] == DLT698_NO_TIME_TAG` 时允许通过，否则返回 `-1`。
  - 成功时若 `consumed != NULL`，写入 `*consumed = off` 并返回 `0`。

## 2. 不变量

- 对任意有界输入长度 `0 <= len <= FORMAL_MAX_LEN`（`12`，以尾部对齐固定缓冲 `apdu = buf + (FORMAL_MAX_LEN - len)` 保持 `apdu[len]` 严格越界，并满足 `unwind = 13 >= max_len + 1` 与嵌套循环 `i < count` x `j < related_count` 的求解预算）及任意符号化字节内容，`dlt698_decode_get_record_request` 不发生数组越界、空/野指针解引用、除零、有符号溢出或非法整数转换。
- 返回值 `rc` 始终属于 `{0, -1, DLT698_RECORD_UNSUPPORTED_SELECTOR, DLT698_RECORD_UNSUPPORTED_DATA}`（头文件契约包含 `DLT698_RECORD_UNSUPPORTED_DATA`）。
- 当 `rc == 0` 时：
  - `consumed <= len`。
  - `rcsd.count <= DLT698_RCSD_MAX`。
  - 对每个 `i < rcsd.count`，当 `rcsd.items[i].kind == DLT698_CSD_ROAD` 时，`rcsd.items[i].related_count <= DLT698_ROAD_MAX`。
- 可达性：`rc == 0` 路径非空虚（`__CPROVER_cover(rc == 0)` 可满足）。
- 具体选择器契约（`selector_reject`）：合法 Selector 1 + RCSD 请求（24 字节）返回 `0`；不支持的选择器返回 `DLT698_RECORD_UNSUPPORTED_SELECTOR`；截断的 Selector 2 请求返回 `-1`。

## 3. 可观测输出

- `formal/manifest/dcu.tsv` 新增 3 条 `owner=dcu`、`sync=3e069e7c` 的记录（`proof`、`cover`、`function`）：
  - `proof` 模式：`--bounds-check --pointer-check --div-by-zero-check --signed-overflow-check --conversion-check -DFORMAL_MAX_LEN=12 --unwind 13 --unwinding-assertions` 报告 `0 of <property_count> failed` 与 `VERIFICATION SUCCESSFUL`。
  - `cover` 模式：`-DFORMAL_MAX_LEN=12 --cover cover --unwind 13` 报告 `[main.coverage.1] ...: SATISFIED`。
  - `function` 模式（`--function selector_reject`）：合法 Selector 1 接受、不支持选择器拒绝、截断 Selector 2 拒绝三项断言均为 `SUCCESS`。
- 负向变异脚本 `formal/cbmc/negative/dlt698_get_record_mutation.sh`：移除 `foundation/src/protocol/dlt698_codec.c` 临时副本中的 `if (len - off < (size_t)csd->related_count * 4) return -1;` 守卫后，CBMC 以非 0 退出并捕获真实越界 `FAILURE`。

## 4. 验收判据

- `python3 ci/scripts/formal_manifest.py --root . --rows` 退出码 `0`。
- `proof`、`cover`、`selector_reject` 三条 CBMC 命令全部通过，且 `formal/manifest/dcu.tsv` 中 `property_count` 与实际输出完全一致。
- `ROOT_DIR=/tmp/dcu-FORMAL-02 FORMAL_OUT_DIR=/tmp/dcu-formal-02-neg CBMC_BIN=$(which cbmc) ARTIFACT=formal/cbmc/dlt698_get_record.c bash formal/cbmc/negative/dlt698_get_record_mutation.sh` 退出码 `0`，稳定捕获越界 `FAILURE` 且不污染工作区。
- `python3 ci/scripts/check_doc_references.py --root .` 退出码 `0`。

## 5. 非目标

- 不修改生产源码 `foundation/src/protocol/dlt698_codec.c`（除非 CBMC 发现真实缺陷；若发现仅记录 finding，不放宽断言）。
- 不验证 `dlt698_encode_get_record_response_begin`、`dlt698_decode_data` 或其他编解码函数。
- 不将调用方传入 `NULL` 指针的边界作为本次符号化 harness 的主路径（由常规单元测试覆盖）。
