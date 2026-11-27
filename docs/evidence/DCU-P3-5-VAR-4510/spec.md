# DCU-P3-5-VAR-4510 规格：dcu_698_value_for_oad 对 4510 A5 的标量口径

分支 `dcu/P3-5-var-4510`，worktree `/tmp/edgec-land/p3-5-var-4510`，基线 `40d8dca5`
（与 `feat/dcu` 一致）。本规格先于代码与测试落盘。

## 0. 代码与标准事实（只读取证）

- OAD 组成：`OAD = OI << 16 | 属性 << 8 | 元素索引`。目标 OAD 为 `0x45100500`
  = `OI_ETHERNET_COMM_FIRST`（`0x4510`，`cmd/dcu/dcu_router.h:118`）+ 属性 A5 + 元素 0。
- 标准 7.3.26 表199：Class 26 属性 A5 是“MAC 地址”，`octet-string`（`static`）；
  属性 A1 逻辑名与 A5 同为 octet-string，A2/A4 是 structure，A3 是 array
  （`docs/698_parsed.md:5616-5675`）。对象表只广告 `4510` 的 A5
  （`cmd/dcu/dcu_698_object_model.c:240-243`）。
- `dcu_value_t`（`cmd/dcu/dcu_698_var.h`）是标量便捷结构：`type_tag/width/raw/value/
  ts_ms/quality`。其 `type_tag` 集合（0x03/0x05/0x06/0x0F/0x10/0x11/0x12/0x15/0x16/0x17）
  没有 octet-string（0x09）；`dcu_698_tag_width()` 只返回标量宽度；
  `_encode_axdr_value()`（`cmd/dcu/dcu_698_axdr.c:87`）也没有 octet-string 编码。
  **结论：标量端口不能承载 6 字节 MAC；不得把 6 字节塞进 `raw` 后截断或伪造。**
- 主站 GET 路径：`dcu_north_resolve_axdr()`（`cmd/dcu/dcu_698_axdr.c:789`）在
  Class 25/26 OI 范围（`:800-803`）先分派 `dcu_comm_info_encode_axdr()`；后者只在
  4510 A5 且 `mac_present` 时编码 octet-string(6)，其余按类定义/未定义回 DAR 5/4
  （`cmd/dcu/dcu_comm_info.c:137-158`）。`:921` 的标量回落对 4510 不可达。
- 当前缺陷：`dcu_698_value_for_oad()`（`cmd/dcu/dcu_698_var.c:330`）对 4510 A5 落入
  “属性已广告但没有标量描述”分支（`:343-348`），返回
  `EDGE_SLAVE_SRC_TYPE_MISMATCH`（-22 → DAR 7）。
- DAR 映射：`foundation/include/edge_slave_plane.h:67-68`；
  `adapters/io_drivers/src/driver_dlt698.c:247-250`。

## 1. 业务规则

- R1. `dcu_698_value_for_oad` 的标量端口只承载标量类型。已广告但标量端口无法表示的
  属性必须明确拒绝：不返回 0，不截断，不伪造字节。
- R2. 4510 A5 的唯一值源是 A-XDR resolver 的 `dcu_comm_info` MAC 快照；标量端口没有
  该属性的标量来源。按 `docs/design/DCU_698_DEVIATIONS.md` Class 25/26 行
  “GET 无来源回 DAR 5”与规则 4“缺数据源或硬件映射时……先回 DAR 5/2，不造值”，
  `dcu_698_value_for_oad(0x45100500)` 返回 `EDGE_SLAVE_SRC_CLASS_MISMATCH`
  （-21 → DAR 5），与平台 MAC 在快照中是否存在无关：标量端口没有来源，
  MAC 是否可用由 A-XDR resolver 单独决定。
- R3. DAR 7（`EDGE_SLAVE_SRC_TYPE_MISMATCH`）保留给线上 A-XDR 类型/宽度不匹配，
  不用于“内部标量便捷结构不支持某个标准类型”。
- R4. 主站 GET 4510 A5 线上行为不变：`mac_present` 时返回 octet-string(6) + MAC
  字节；缺失时 DAR 5。SET 仍 DAR 3。
- R5. 其它 Class 25/26 OAD 的标量 DAR 不变：类定义但无来源 DAR 5（4500/4501/4511–4517、
  4510 A1–A4），类未定义属性 DAR 4（如 4510 A6），未知 OI DAR 6。

## 2. 不变量

- I1. `dcu_value_t` 不新增成员或编码；不引入 octet-string `type_tag`。
- I2. `dcu_698_value_for_oad` 返回 0 时，`out->type_tag` 属于现有标量集合，且
  `out->width == dcu_698_tag_width(out->type_tag)`。
- I3. `dcu_north_resolve_axdr(0x45100500)` 在 `mac_present` 时输出 `len=8`、tag `0x09`、
  长度字节 `6`、后 6 字节等于快照 MAC；`mac_present=false` 时返回
  `EDGE_SLAVE_SRC_CLASS_MISMATCH`。
- I4. 修复只改变 `dcu_698_value_for_oad(0x45100500)` 的返回码；其它任何输入的
  返回码不变。

## 3. 可观测输出

- O1. `dcu_698_value_for_oad(MK_OAD(0x4510, 5), &v) == EDGE_SLAVE_SRC_CLASS_MISMATCH`
  （-21，线上 DAR 5），无论 MAC 快照有无。
- O2. 主站 GET 4510 A5（MAC 快照 `02:11:22:33:44:55`）：
  `rc=0`、`len=8`、字节 `09 06 02 11 22 33 44 55`。
- O3. 主站 GET 4510 A5（无 MAC）：`EDGE_SLAVE_SRC_CLASS_MISMATCH`（DAR 5）。
- O4. 边界 DAR：`value_for_oad(4510 A6) == EDGE_SLAVE_SRC_UNDEFINED`（-23 → DAR 4）；
  `value_for_oad(4500 A1) == EDGE_SLAVE_SRC_CLASS_MISMATCH`（DAR 5）；
  `value_for_oad(4511 A5) == EDGE_SLAVE_SRC_CLASS_MISMATCH`（DAR 5）。
- O5. 修复前 O1 实际为 `EDGE_SLAVE_SRC_TYPE_MISMATCH`（-22 → DAR 7），作为红证据。

## 4. 验收判据（测试断言映射）

| 判据 | 规格条目 | 期望 | 测试断言 |
| --- | --- | --- | --- |
| A1 | R1/R2/O1 | 标量路径对 4510 A5 回 DAR 5，不因 MAC 存在而变 | `assert_int_equal(dcu_698_value_for_oad(MK_OAD(0x4510,5), &v), EDGE_SLAVE_SRC_CLASS_MISMATCH)`（安装 MAC 快照后） |
| A2 | R4/I3/O2 | 主站 GET 保持 octet-string(6) 与 MAC 字节 | 同测试：`dcu_north_resolve_axdr(...,0x45100500,...)` rc 0、len 8、`09 06 02 11 22 33 44 55` |
| A3 | R2/R4/O1/O3 | 无 MAC 时标量与主站 GET 都回 DAR 5 | `dcu_comm_info_reset()` 后同两断言回 `EDGE_SLAVE_SRC_CLASS_MISMATCH` |
| A4 | R5/O4 | 类未定义 DAR 4、无来源类属性 DAR 5 不被本次改动泛化 | 4510 A6、4500 A1、4511 A5 三条断言 |
| A5 | R3/I4 | 标量端口不新增 octet 能力，其它返回码不变 | 代码审查 + 定向套件回归；不新增 `type_tag` |
| A6 | 完成定义 | 定向 `ctest -R 'dcu|dlt698'` 全绿；验收 dcu 2/2 | 见 summary.md 原始输出与计数 |

## 5. 非目标

- 不扩展 `dcu_value_t` 或标量端口以承载 octet-string/visible-string。
- 不修改 `dcu_comm_info.c`、`dcu_698_axdr.c`、`dcu_698_object_model.c` 与 ACL。
- 不实现 MAC 写/持久化、多以太网口映射、APN/IP/信号等 Producer。
- 不改主站 GET/SET/ACTION 线上语义，不改其它 OAD 的 DAR。
- 不跑全量 ctest；不新增代码注释。
