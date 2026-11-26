# DCU-FIX-578-1：GetRecord 整页溢出从站缓冲

## 结论

255 行整页（5379 字节）写入 4096 字节从站缓冲时，编码层不查 tailroom；Release（`-DNDEBUG`）把 `net_buf_simple_add` 的 `assert` 编译掉，驱动仍回 `rc=0`，实测写出 cap 1283 字节。修复后整页放不下（包括末尾 5 字节 trailer）时回退到 `stream.start_len`，按 DAR 2（暂时失效）应答；`begin`、`row`、`end`、DAR 四个编码点都在写入前检查 tailroom，绝不写越 cap。

验证方第二轮发现：行放得下、但末尾 5 字节放不下的页（如 194×21）会静默（`rc=-1`）。已在本轮修复：行编码预留 trailer；`end` 仍失败时驱动回退并编码 DAR 2。

## 根因

- `foundation/src/net_buf.c:201`：`net_buf_simple_add` 只有 `assert(len <= net_buf_simple_tailroom(buf))`。
- `foundation/src/protocol/dlt698_codec.c`：`dlt698_encode_get_record_row` 直接 `net_buf_simple_add_mem`，没有容量检查；行检查只算行自身，不预留 trailer。
- `adapters/io_drivers/src/driver_dlt698.c`：`_handle_get_record` 把行编码失败当 `-1`（不应答）；`end` 失败也直接 `return`，没有按整页失败回退。

## 修复

- `dlt698_codec.c`：新增 `_get_record_prefix_len`，预检 begin/DAR 固定前缀；新增 `DLT698_GET_RECORD_TRAILER_LEN`（5 = FollowReport + 时间标签 + FCS + END）；row 检查 `row_len + trailer <= tailroom`；end 预检 trailer；DAR 编码自检，回退路径本身也不能溢出。
- `driver_dlt698.c`：row 编码失败时 `src->record.end`、`out->b.len = stream.start_len`，编码 DAR 2；`end` 失败同样回退并编码 DAR 2，不再静默。
- 行为变化：合法 255 行整页与"行放得下但 trailer 放不下"的页，从"溢出 rc=0 / 静默"改为"DAR 2 如实失败"。分帧传输不在本次范围。

## 完成定义

- 静态分析：`check_build_warnings.py` 通过（tests.log 末段，0 violations）。
- 测试：`tests/test_dcu_698_record.c`
  - `test_record_page_overflow_returns_dar2`：真实 `dcu_698_record` 端口 + 驱动，4096 缓冲，255 行。
    修复前 `out.b.len <= OVERFLOW_CAP is not true`（len=5379）；修复后 9/9 通过。
  - `test_record_page_trailer_boundary_returns_dar2`：假记录源，194×21、163×25、151×27（前缀 19），
    修复前（f0a1b1ec）`-1 != 0`，修复后 rc=0、DAR 2、canary 干净。
  - ASAN：两轮均通过（见 tests.log）。
  - 相邻回归：`test_dlt698_conformance_suite`、`test_driver_dlt698`、`test_dcu_698_axdr`、
    `test_dlt698_get_list`、`test_dlt698_axdr` 与 `test_dcu_698_record` 共 6/6 通过。
- 运行时：原 repro（`/tmp/interrogate-578/adv-2/repro_record_overflow.c`）
  - 前：`declared capacity=4096 response len=5379 bytes past capacity=1283`。
  - 后：`declared capacity=4096 response len=24 bytes past capacity=0`，帧解码 `rc=0`。
- 运行时：验证方边界 sweep（`/tmp/verify/fix1_boundary.c`）
  - 前（f0a1b1ec）：`row=21 total=194 rc=-1 len=4093 over=0 dar=-1` 等 4 例；`first_bad=21194`。
  - 后：同 4 例 `rc=0 len=24 over=0 dar=2`；`first_bad=0 first_overrun=0`。
- 证据文件：`tests.log`、`runtime.log`、`diffstat.txt`、`comments.md`。

## 未决项

- `begin` 预检失败仍回 `-1`（无应答）。此时连响应帧头都放不下，DAR 响应属于同一尺寸量级，静默是唯一可行结果；4096 缓冲下前缀约 19 字节，不可达。
- 独立 Comment Sicko 的开放项（不阻塞本缺陷，留作后续）：
  1. `_get_record_prefix_len` 与帧写入器是两处算术，字段变化时需同步；当前逐项相等（OAD 5 字节；ROAD 6 + related_count*4）。
  2. `OVERFLOW_CAP` 写死 4096，镜像 `cmd/dcu/main.c:171` 的 hub 池 `buf_size`；建议共享常量。
  3. `mk_acl_read` 裸 A-XDR tag，建议改用 `DLT698_TAG_*` 与 `OAD_DAILY_FREEZE`。
  4. 两处 GET-Request 字面量重复，建议用常量合成 builder。
  5. 边界三元组 194/21、163/25、151/27 建议从 cap 与开销推导。
  6. 补测：ROAD RCSD 分支；零行页的 `end` 失败回退。
- 详见 `comments.md` 第 3 轮。trailer 的 5 已抽成宏。

## 提交

- `8da95fc9` test: 失败复现（先失败）
- `fe9dffa4` fix: 容量预检 + 整页 DAR 2 回退
- `8808f489` style: /no-comments 接受的注释精简
- `f0a1b1ec` docs: 首轮证据包
- `5d014fbc` test: trailer 边界复现（先失败）
- `a2b5f0e6` fix: 预留 trailer + end 失败回退 DAR 2
- 其后的 docs 提交：本轮证据更新
- 独立 Comment Sicko 的注释精简与证据修正：见 `comments.md` 第 3 轮

分支 `dcu/fix-578-1-getrecord-overflow`，worktree `/tmp/edgec-land/fix1-getrecord-overflow`。未合并，等 dcu-verifier 验证。
