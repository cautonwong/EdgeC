# DCU-FIX-578-1：/no-comments（Comment Sicko）

范围：`git diff 45cdc244..HEAD`。代码 3 个文件；证据包 4 个文件。以下第 3 轮是独立 Comment Sicko 后的当前状态，前两轮的"已恢复/保留"描述已被本轮取代。

## 第 1 轮（修复方）

- 删除测试里的整页回退说明，保留一行生产 cap 事实。已提交 `8808f489`。
- 拒绝删除 `dlt698_encode_get_record_response_end` 的 trailer 映射注释；已恢复。
  trailer 修复轮把字面量 `5` 抽成 `DLT698_GET_RECORD_TRAILER_LEN`，原行内注释改为宏注释。

## 第 2 轮（修复方）

- 保留：`Release -DNDEBUG` 编译掉 assert 的约束注释；A-XDR 字节标注；`_handle_get_record` 既有整页失败注释。
- 拒绝两处删除建议：前缀 helper 说明；测试的请求/响应布局注释。
- MUST KILL 报告为开放：前缀算术与帧写入器单一真源；`OVERFLOW_CAP` 产品/测试共享常量。跨文件重构，超出本缺陷范围。

## 第 3 轮（独立 Comment Sicko，当前状态）

本轮删除 14 处注释文本，只保留 3 处有证明的注释。

保留：

- `dlt698_codec.c`：`DLT698_GET_RECORD_TRAILER_LEN` 的组成说明（FollowReport + 时间标签 + FCS + END）。这是调用点唯一看到 `5` 的地方，FCS/END 由 `_frame_seal_offsets` 在预检之后追加。
- `dlt698_codec.c`：`Release -DNDEBUG` 把 `net_buf_simple_add` 的 assert 编译掉，写入前必须查 tailroom。
- `tests/test_dcu_698_record.c`：DAR 响应 `payload = OAD | RCSD | 结果选择 | DAR | FollowReport | 时间标签` 的协议布局。没有结构化访问器，裸下标不可自解释。

删除：

- 1 处 trailer 预留的复述，1 处前缀 helper 名的复述。
- 测试里 8 处 A-XDR tag/ACL 标注（array/structure/attr id/rights/methods），代码可用 `DLT698_TAG_*` 与 `OAD_DAILY_FREEZE` 表达。
- 测试里的生产 cap 说明、请求 APDU 布局说明、trailer 边界意图说明、前缀 19 的边界推导说明。
- 这些删除只动注释文本，不改应用代码、断言或测试输入。

## 开放项（未改，留作后续）

1. `_get_record_prefix_len` 与 `_frame_begin`/`_add_rcsd` 是两处算术。当前逐项相等（OAD 5 字节；ROAD 6 + related_count*4 字节），字段变化时必须同步。建议抽单一长度来源。
2. `OVERFLOW_CAP` 写死 4096，镜像 `cmd/dcu/main.c:171` 的 hub 池 `buf_size`。建议产品与测试共享常量。
3. `mk_acl_read` 用裸 A-XDR tag 拼对象列表。建议改用 `DLT698_TAG_ARRAY/_STRUCTURE/_UNSIGNED/_ENUM` 与 `OAD_DAILY_FREEZE` 派生。
4. 两处 GET-Request 字面量 `{0x50,0x04,0x02,0x00,0x00,0x00,0x00}` 重复。建议用 `DLT698_SVC_GET_REQ`、`DLT698_CHOICE_GET_RECORD`、`OAD_DAILY_FREEZE`、`DLT698_NO_TIME_TAG` 合成一个 builder。
5. 194×21、163×25、151×27 是 magic 边界三元组。建议从 cap 与响应开销推导。
6. 覆盖缺口：ROAD RCSD 分支（`related_count > 0`）没有用例；零行页的 `end` 失败回退没有用例，本轮 194×21 等页在 row 预检就回 DAR 2。这两条不影响本轮修复的正确性，但应补测。
7. `begin` 预检失败仍回 `-1`（无应答）。此时连响应帧头都放不下，4096 缓冲下不可达。

## 验证

- 注释删除后重建 Release `test_dcu_698_record`：9/9 通过。
- ASAN `test_dcu_698_record`：9/9 通过。
- 验证方边界 sweep：`first_bad=0 first_overrun=0`，row=21/total=194、25/163、27/151 均回 DAR 2，canary 干净。

结论：应用代码、断言、测试输入未因注释审查改变；开放项均为后续重构或补测，不阻塞本缺陷。
