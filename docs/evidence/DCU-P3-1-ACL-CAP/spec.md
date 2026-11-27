# DCU-P3-1-ACL-CAP spec（先于实现/测试的独立预期）

基线：`feat/dcu` `9d065b6d`；分支 `dcu/P3-1-acl-cap`。本 spec 由任务单与
`docs/design/DCU_698_OBJECT_MODEL.md` §8.5、计划 §12.5、`DCU_698_DEVIATIONS.md`
的既有口径推导，测试与实现按条目编号引用。

## 五栏

| 业务规则 | 不变量 | 可观测输出 | 验收判据 | 非目标 |
| --- | --- | --- | --- | --- |
| S1 任务单 HOLD 清单里“类定义但每对象槽不足”的属性/方法必须进 Class 20 ACL（只读广告）：4307/4308/4309 方法 127/128/129；F100 方法 1..12；2301..2308 属性 1..18；F001 方法 1..10。4300 没有 OI 专属方法，127..129 不广告；F000/F002 方法 9/10 不广告。 | ACL 每对象描述 12 B；属性权限 2 bit/属性覆盖 1..18（5 B）；方法号放会话级 1024 B 池，单对象最多 12 个方法号；对象数仍 224。 | 对象列表 A-XDR 中对应 OI 的属性/方法条目出现且 rights=1；主站 GET/ACTION 越过 ACL，按 resolver 语义回值。 | `test_class19_devmgr_acl_is_read_only`、`test_class21_esam_acl_is_read_only`、`test_class23_group_acl_is_read_only`、`test_class18_file_acl_is_read_only` 断言条目存在与 rights=1；对应用例断言主站 DAR 5。 | 不实现这些属性/方法的生产者；无来源仍回 DAR 5，不伪造值。 |
| S2 对象列表解析保持 fail closed：属性 id>18、单对象方法数>12、对象数>224、累计方法号超过 1024 B 池，任一超限都拒绝整个列表。 | 不截断、不越界、不部分生效；ACL 不可用时所有请求按未授权 DAR 3。 | 超限列表下主站 GET 回 DAR 3。 | `test_slave_rejects_over_cap_object_list`（225 对象）、`test_slave_rejects_acl_method_pool_overflow`（1200 方法号）。 | 不扩 `EDGE_SLAVE_ACL_MAX_OBJECTS`/`EDGE_SLAVE_AXDR_MAX`/`DRIVER_PRIV_SIZE`。 |
| S3 ACL 优先语义不变：已广告且类定义无来源的 GET/ACTION 回 resolver DAR 5；只读广告属性的 SET 回 ACL DAR 3；类未定义或未广告一律 DAR 3。 | ACL 判定在 resolver 之前；广告集合 ⊆ resolver 定义集合（4300 127..129、F000/F002 9/10 不广告）。 | 主站 SET/GET/ACTION 的 DAR 数值。 | `test_slave_class19_devmgr_dar`、`test_slave_class21_esam_dar`、`test_slave_class23_group_dar`、`test_slave_class18_file_dar` 的 5/3 断言；负例：4300 方法 127、F100 方法 13、2301 A19、F000/F002 方法 9 仍 DAR 3；2301 A17 SET DAR 3。 | 不改 resolver 的 DAR 映射；不把 DAR 3 改成成功。 |
| S4 预算不变量：`sizeof(dlt698_session_t) <= DRIVER_PRIV_SIZE`（4096 B 不变）；对象列表数据 ≤ `EDGE_SLAVE_AXDR_MAX` 10240 B，帧 ≤ `BUS_SLAVE_POOL_BUF_SIZE` 10496 B；224 对象长格式 ACL 仍可解析。 | 会话结构 3752 B（余 344 B）；对象列表 5992 B 数据 / 6014 B 帧（余 4482 B）；D1-21 的 224 项投影余量不变。 | `dlt698_driver.ctx_size`；对象列表响应字节数；4096 B 池下 DAR 2；224 项 ACL 的末项可 GET。 | `test_slave_object_list_fits_production_slave_pool`（6014）、`test_slave_object_list_does_not_overflow_pool_buffer`、`test_slave_parses_long_form_acl_224`；`budgets.txt` 的 gdb 实测。 | 不抬 `DRIVER_PRIV_SIZE`/`EDGE_SLAVE_AXDR_MAX`/`EDGE_SLAVE_ACL_MAX_OBJECTS`。 |
| S5 既有 HOLD 口径同步：Class 12 2401 A16/A17 是标准归属冲突、与本上限无关；A18/A19 未裁决，继续只广告 A1..A15。 | 2401 的 ACL 条目保持 15 个。 | 2401 A16 主站 GET 仍 DAR 3。 | `test_class12_pulse_acl_is_read_only`（既有断言不变）。 | 不在本单裁决表157/表158 冲突，不扩 2401 广告。 |

## 条目 → 测试/断言映射

| 条目 | 测试文件::测试名 | 关键断言 |
| --- | --- | --- |
| S1 | tests/test_dcu_698_appconn.c::test_class19_devmgr_acl_is_read_only | 4307/4308/4309 `method_seen[127..129]` 且 rights=1；4300 `method_seen[127]` 为假 |
| S1 | tests/test_dcu_698_appconn.c::test_class21_esam_acl_is_read_only | `method_seen[1..12]` 且 rights=1；13 为假 |
| S1 | tests/test_dcu_698_appconn.c::test_class23_group_acl_is_read_only | 每个 2301..2308 `seen[1..18]` 且 rights=1；19 为假 |
| S1 | tests/test_dcu_698_appconn.c::test_class18_file_acl_is_read_only | F001 `method_seen[1..10]`；F000/F002 只到 8 |
| S1/S3 | tests/test_driver_dlt698.c::test_slave_class19_devmgr_dar | 4307..4309 方法 127..129 主站 DAR 5；4300 方法 127 DAR 3 |
| S1/S3 | tests/test_driver_dlt698.c::test_slave_class21_esam_dar | F100 方法 9..12 DAR 5；方法 13 DAR 3；SET A2 DAR 3 |
| S1/S3 | tests/test_driver_dlt698.c::test_slave_class23_group_dar | 2301 A17/A18 GET DAR 5；A19 DAR 3；A17 SET DAR 3 |
| S1/S3 | tests/test_driver_dlt698.c::test_slave_class18_file_dar | F001 方法 9/10 DAR 5；F000/F002 方法 9 DAR 3；SET A2 DAR 3 |
| S2 | tests/test_driver_dlt698.c::test_slave_rejects_over_cap_object_list | 225 对象列表 → DAR 3 |
| S2 | tests/test_driver_dlt698.c::test_slave_rejects_acl_method_pool_overflow | 1200 个方法号的列表 → 整单拒绝，GET DAR 3 |
| S4 | tests/test_driver_dlt698.c::test_slave_object_list_fits_production_slave_pool | 帧长精确 6014 B，118 项 |
| S4 | tests/test_driver_dlt698.c::test_slave_object_list_does_not_overflow_pool_buffer | 4096 B 池装不下 → DAR 2，且池尾不被写 |
| S4 | tests/test_driver_dlt698.c::test_slave_parses_long_form_acl_224 | 224 项长格式 ACL 可解析，末项 GET 进入 resolver，列表外 DAR 3 |
| S4 | budgets.txt（gdb 读 `dlt698_driver.ctx_size`） | 改前 3624 B → 改后 3752 B，≤4096 |
| S5 | tests/test_dcu_698_appconn.c::test_class12_pulse_acl_is_read_only | 2401 只广告 1..15（既有断言） |

## 独立预期（verifier 可复算）

- 对象列表新增条目：Class 23 8 对象 × 2 属性 + Class 19 3 对象 × 3 方法 +
  Class 21 4 方法 + Class 18 2 方法 = 31 项，每项 6 B → +186 B；5806+186=5992 B；
  加 22 B 帧 = 6014 B。
- 会话：每对象 2(oi)+1+1+2(方法池偏移)+5(权限)=12 B → 224×12=2688 B；
  方法池 1024 B；头 38 B；对齐 8 → 3752 B；`DRIVER_PRIV_SIZE` 4096 余 344 B。
- 产品方法号总数：改前 281，新增 15 → 296，占池 1024 的 29%。
- 对象列表大小 5992 B < `EDGE_SLAVE_AXDR_MAX` 10240；帧 6014 B <
  `BUS_SLAVE_POOL_BUF_SIZE` 10496。
