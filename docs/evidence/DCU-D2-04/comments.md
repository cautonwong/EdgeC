# DCU-D2-04 注释审计

范围：`1488eb4f..HEAD` 的实现 diff，含新文件 `dcu_scheme_store.{h,c}`、`tests/test_dcu_698_schemes.c`。

## 删除

16 行（8 处章节横幅 + 相邻空行）：`dcu_698_axdr.c` 的 6014/6015 横幅；方案测试里的 mock、A-XDR 构造、列选择、形状错误、字段策略、A2 字节、持久化 7 个横幅。无平台/协议规则被删。

## 保留

- 协议布局：CSD `5B 00/01` 与 RCSD 的区别、MS `5C choice`、6015 六字段顺序、采集类型 0–4 的内容形状、Data 定长表。
- DAR：形状错 7、产品不执行 2、被引用 3/6、类规则 5/4；解码先于状态修改。
- 持久化：SQL 先写、RAM 后改、回滚快照、只播种一次。
- 方案引用规则与 `item_mask` 运行期语义、`scheme_op` 位于聚合体与 uv 线程所有权。
- ACL 上限必须与驱动的 `DLT698_ACL_MAX_OBJECTS` 一致（已在驱动源码核实，静态断言保留）。

## 说明

- 注释改动后重跑了增量构建与 38 项聚焦测试。

## rebase 后第二轮

删除 7 行叙述：ticket 范围说明、测试旁白与重复块注释、重启步骤复述、种子方案算术说明。保留时间标签/路由/DAR/ACL 上限约束注释。另修两处非注释问题：`test_driver_dlt698.c` 的 `ois[31]` 配 33 项越界读取改为 `ois[33]`；`dcu_north_adapter.c` 的 6000/6012 注释补上 6014。

## verifier 复审计（第三轮）

- 删除 5 处复述/叙述：`dcu_698_axdr.c` 的定长载荷导语与 6015 单元导语、`dcu_north_adapter.c`
  的派发说明、`test_dcu_698_schemes.c` 的两条 fixture 旁白。`_decode_capture_content`
  只留下调用方不变量（类型 > 4 已按 DAR 7 拒绝）。
- MUST KILL 1：ACL 对象上限改为 `edge_slave_plane.h` 的 `EDGE_SLAVE_ACL_MAX_OBJECTS`，
  驱动与 `dcu_698_appconn.c` 的 `_Static_assert` 共用一处，删掉"必须一致"的文字约定。
- MUST KILL 2：`test_dcu_698_schemes.c` 的 `bad_content.b[13]` 改成用 `_add_begin/_u8/
  _csd_oad/_ms_payload` 显式构造错误采集内容，删掉魔数偏移注释。
- 验收 docstring 删掉叙述测试步骤的第一行，保留 H.5.4 profile 偏差说明。
- 保留：6.1.1.5 时间标签规则、A.7 6015 六字段布局、6014 持久化端口的"同一 SQLite 连接"
  设计约束（`dcu_node.c`）、`dcu_scheme_store.c` 的回滚不变量。
- 同时从分支移除误提交的 `.audit/` 草稿目录。
