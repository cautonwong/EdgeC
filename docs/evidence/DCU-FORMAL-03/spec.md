# DCU-FORMAL-03 规格说明：`dlt698_encode_report_notification` CBMC 形式化验证

## 1. 业务规则

- `dlt698_encode_report_notification(buf, cap, choice, piid, items, n, len)` 将主动上报对象列表编码为 `REPORT-Notification` 的 APDU 体（服务标识之后）：`CHOICE(1) | PIID-ACD(1) | 个数(1) | (OAD(4) | Get-Result)*`，并写入 `*len`。
- 参数与选择分支判断：
  - `buf`、`len` 任一为 `NULL`，或 `n > 0 && items == NULL` 时返回 `-1`。
  - `choice == DLT698_REPORT_RECORD_LIST`（`0x02`）时返回 `DLT698_REPORT_UNSUPPORTED_RECORD`（`-4`）。
  - `choice != DLT698_REPORT_LIST`（`0x01`）或 `n > DLT698_GET_LIST_MAX`（`8`）时返回 `-1`。
- 预计算总长与空指针检查：
  - `need` 初始为 `3`（`CHOICE | PIID | 个数`）。
  - 遍历 `i = 0 .. n - 1`：若 `items[i].dar == 0 && items[i].dlen > 0 && items[i].data == NULL` 返回 `-1`；累加 `_report_item_len(&items[i]) = 4 + 1 + (dar ? 1 : dlen)`。
  - 若 `need > cap` 返回 `-1`（先算总长再写，防止裸缓冲写越界）。
- 编码输出：
  - 写入 `DLT698_REPORT_LIST`、`piid`、`(uint8_t)n`。
  - 逐项写入大端 4 字节 `items[i].oad`；若 `items[i].dar != 0` 写入 `0x00 | items[i].dar`（`CHOICE [0] DAR`）；否则写入 `DLT698_GET_RESULT_DATA`（`0x01`）并在 `dlen > 0` 时拷贝 `dlen` 字节 `items[i].data`。
  - 写入 `*len = off` 并返回 `0`。

## 2. 不变量

- 对任意有界缓冲容量 `0 <= cap <= FORMAL_MAX_LEN`（`12`，以尾部对齐固定缓冲 `buf = out_buf + (FORMAL_MAX_LEN - cap)` 保持 `buf[cap]` 严格越界，并满足 `--unwind 13 >= max_len + 1`）、任意符号化 `choice`、`piid`、`n` 及每项有界载荷（`dlen <= 8`），`dlt698_encode_report_notification` 不发生数组越界、空/野指针解引用、除零、有符号溢出或非法整数转换。
- 返回值 `rc` 始终属于 `{0, -1, DLT698_REPORT_UNSUPPORTED_RECORD}`。
- 当 `rc == 0` 时：`len <= cap`。
- 可达性：`rc == 0` 路径非空虚（`__CPROVER_cover(rc == 0)` 为 `SATISFIED`）。
- 具体拒绝与接受契约（`report_reject`）：
  - 合法 `DLT698_REPORT_LIST` 请求返回 `0` 且 `len == 10`（1 项 2 字节 Data：`3 + 4 + 1 + 2`）。
  - `DLT698_REPORT_RECORD_LIST` 返回 `DLT698_REPORT_UNSUPPORTED_RECORD`。
  - 容量不足（`cap < need`）返回 `-1`。

## 3. 可观测输出

- `formal/manifest/dcu.tsv` 新增 3 条 `owner=dcu`、`sync=5815f344` 的记录（`proof`、`cover`、`function`）：
  - `proof` 模式：`--bounds-check --pointer-check --div-by-zero-check --signed-overflow-check --conversion-check -DFORMAL_MAX_LEN=12 --unwind 13 --unwinding-assertions` 报告 `0 of <property_count> failed` 与 `VERIFICATION SUCCESSFUL`。
  - `cover` 模式：`-DFORMAL_MAX_LEN=12 --cover cover --unwind 13` 报告 `[main.coverage.1] ...: SATISFIED`。
  - `function` 模式（`--function report_reject`）：合法 `DLT698_REPORT_LIST` 接受、`DLT698_REPORT_RECORD_LIST` 拒绝、容量不足拒绝三项断言均为 `SUCCESS`。
- 负向变异脚本 `formal/cbmc/negative/dlt698_encode_report_mutation.sh`：移除 `foundation/src/protocol/dlt698_codec.c` 临时副本中的 `if (need > cap) return -1;` 守卫后，CBMC 以非 0 退出并捕获真实越界写 `FAILURE`（`< 60s`）。

## 4. 验收判据

- `python3 ci/scripts/formal_manifest.py --root . --rows` 退出码 `0`。
- `proof`、`cover`、`report_reject` 三条 CBMC 命令全部通过，且 `formal/manifest/dcu.tsv` 中 `property_count` 与实际输出一致。
- `formal/cbmc/negative/dlt698_encode_report_mutation.sh` 退出码 `0`，稳定捕获越界写 `FAILURE` 且不污染工作区。
- `python3 ci/scripts/check_doc_references.py --root .` 退出码 `0`。

## 5. 非目标

- 不修改生产源码 `foundation/src/protocol/dlt698_codec.c`（除非 CBMC 发现真实缺陷；若发现仅记录 finding，不放宽断言）。
- 不验证 `dlt698_encode_report_frame`、`dlt698_decode_data` 或其他编解码函数。
- 不将调用方传入 `NULL` 指针或 `dlen > 8` 的大载荷作为本次符号化 harness 的主路径（由常规单元测试覆盖）。
