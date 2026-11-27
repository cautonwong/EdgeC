# DCU-FORMAL-01 规格说明：`dlt698_decode_frame_ex` CBMC 形式化验证

## 1. 业务规则

- `dlt698_decode_frame_ex(in, sa, svc, choice, piid, payload_out, payload_len)` 接收非空输入缓冲 `in` 与非空输出指针 `sa`、`svc`、`piid`、`payload_out`、`payload_len`（`choice` 可为 `NULL`）。
- 长度不足判断：
  - `in->len < 12` 时返回 `1`（帧不完整）。
  - 按小端解析长度域 `l_field = in->data[1] | (in->data[2] << 8)`，`frame_len = l_field + 1`；若 `in->len < frame_len + 1` 返回 `1`。
  - 解析地址域长度 `sa_len = (in->data[4] & 0x0F) + 1`；若 `frame_len < 13 + sa_len` 返回 `1`。
- 帧定界判断：
  - `in->data[0] != 0x68` 或 `in->data[frame_len] != 0x16` 时返回 `-2`。
- 校验判断：
  - 先以 `dlt698_calculate_fcs(in->data + 1, 5 + sa_len)` 校验位于 `6 + sa_len` 的 2 字节 HCS；不匹配返回 `-1`。
  - 再以 `dlt698_calculate_fcs(in->data + 1, frame_len - 3)` 校验位于 `frame_len - 2` 的 2 字节 FCS；不匹配返回 `-1`。
- 成功解码（返回 `0`）：
  - 写入 `sa->type = in->data[4] >> 6`、`sa->len = sa_len` 并复制 `sa_len` 字节地址到 `sa->addr`。
  - 提取 `*svc = apdu_start[0]`；对 `0x02`/`0x03` 无 CHOICE 服务置 `*choice = 0`（若 `choice != NULL`）且 `*piid = apdu_start[1]`、载荷起始偏移 `+2`；其余服务置 `*choice = apdu_start[1]`、`*piid = apdu_start[2]`、载荷起始偏移 `+3`。
  - `*payload_len = (in->data + frame_len - 2) - (*payload_out)`。

## 2. 不变量

- 对任意有界输入长度 `0 <= len <= FORMAL_MAX_LEN` 及任意符号化字节内容，`dlt698_decode_frame_ex` 不发生数组越界、空/野指针解引用、除零、有符号溢出或非法整数转换。
- 返回值 `rc` 始终属于 `{0, 1, -1, -2}`。
- 当 `rc == 0` 时：
  - `sa.len` 满足 `1 <= sa.len <= 16`。
  - `payload_out` 严格落在 `[buf, buf + len)` 内。
  - `payload_len <= len` 且 `(size_t)(payload_out - buf) + payload_len < len - 2`（载荷严格位于尾部 2 字节 FCS 与 1 字节 `0x16` 之前）。
- 可达性：`rc == 0` 路径非空虚（`__CPROVER_cover(rc == 0)` 可满足）。
- 校验敏感性：合法 DL/T 698.45 帧返回 `0`；翻转 HCS 或 FCS 任意 1 bit 返回 `< 0`。

## 3. 可观测输出

- `formal/manifest/dcu.tsv` 中的 3 条 `owner=dcu` 记录（`proof`、`cover`、`function`）：
  - `proof` 模式：`--bounds-check --pointer-check --div-by-zero-check --signed-overflow-check --conversion-check --unwind 33 --unwinding-assertions` 报告 `0 of <property_count> failed` 与 `VERIFICATION SUCCESSFUL`。
  - `cover` 模式：`--cover cover --unwind 33` 报告 `[main.coverage.1] ...: SATISFIED`。
  - `function` 模式（`--function fcs_reject`）：合法帧、HCS 翻转拒绝、FCS 翻转拒绝三项断言均为 `SUCCESS`。
- 负向变异脚本 `formal/cbmc/negative/dlt698_guard_mutation.sh`：移除 `dlt698_codec.c` 临时副本中的 `in->len < frame_len + 1` 守卫后，CBMC 以非 0 退出并捕获 `dlt698_decode_frame_ex` 的真实越界 `FAILURE`。

## 4. 验收判据

- `./ci/scripts/check_formal.sh --strict` 退出码 `0`，覆盖全部 `iems` 与 `dcu` 分片且 `property_count` 与实际一致。
- `./ci/scripts/check_formal_negative.sh --strict` 退出码 `0`，`dlt698_guard_mutation.sh` 稳定触发并捕获越界失败，且仓库工作区保持干净。
- `python3 ci/scripts/run_guard_selftest.py` 与 `python3 ci/scripts/check_doc_references.py --root .` 均以退出码 `0` 通过。
- `/no-comments`（`comment-sicko`）对本次新增/修改行复核通过并留存 `docs/evidence/DCU-FORMAL-01/comments.md`。

## 5. 非目标

- 不修改生产源码 `foundation/src/protocol/dlt698_codec.c`（除非 CBMC 发现真实缺陷；若发现仅记录 finding，不放宽断言）。
- 不验证 `dlt698_decode_data`、记录型读解码或编码函数。
- 不将 `in == NULL`、`sa == NULL`、`svc == NULL`、`piid == NULL`、`payload_out == NULL`、`payload_len == NULL` 纳入本次契约（调用方保证非空）。
