# DCU-D5-03 Class 11 集合（DAR 诚实骨架，7000/7010）

分支 `dcu/D5-03-class11-collection`，工作树 `/tmp/edgec-land/d5-03-class11`，基线 `feat/dcu`
`f25d390f`（D5-06 已合入）。

## 前提核查与标准冲突

- 7.3.11 表154–156（`docs/698_parsed.md:4297-4336`）：Class 11 定义属性 1 逻辑名、
  2 集合 array、3 当前元素个数、4 最大元素个数与方法 1 复位、2 执行（均可选）。
- A.8（`docs/698_parsed.md:8632-8647`）的 7000 文件集合只列方法：127 WriteFile（structure）、
  128 Execute（文件名）、129 DeleteFile（文件名），**没有属性行**。
- A.8（`docs/698_parsed.md:8668-8678`）的 7010 脚本集合有 A2 `array 脚本` 与方法
  127 Add、128 Delete、129 Execute、130 Clear；脚本 id 是 long-unsigned。
- 计划 §8 D5-03 的出口写“GET 7000 返回集合元素”，与 A.8 冲突：7000 只有方法，A2 在 7010。
  本单按标准改为“7010 A2 DAR 5 + 7000 方法 127–129 DAR 5”的诚实骨架，偏差记录见下，
  计划由 verifier 更新。

产品没有文件/脚本集合存储（D4-06 仅覆盖 Class 18 文件传输且未做，`dcu_698_set.c` 不存在），没有集合元素数据源。
对象列表与 ACL 的唯一来源是 `dcu_698_appconn_object_list()`（ADR-0015）。

## 落地的数据形状

| OI | 接口类 | 范围 | 对象 attr mask | 对象 method mask | ACL |
| --- | --- | --- | --- | --- | --- |
| 7000 文件集合 | IC_COLLECTION (11) | 终端级 | 空 | 空 | 无属性；方法 127–129 |
| 7010 脚本集合 | IC_COLLECTION (11) | 终端级 | 空 | 空 | A2 只读（rights 1）；方法 127–130 |

对象行（`dcu_698_object_model.c`，`OI_FILE_SET` / `OI_SCRIPT_SET`）只广告对象存在性；
`instance` 为 NULL，没有 read/write/method 处理器。ACL（`dcu_698_appconn.c`）是只读子集，
主站请求先过 ACL，再进对象表。

## DAR 矩阵（resolver 层与主站层）

| 请求 | resolver 层 | 主站层（先过 ACL） |
| --- | --- | --- |
| GET 7010 A2（类定义、无来源） | DAR 5 | DAR 5（A2 在 ACL） |
| GET 7010 A5（类未定义） | DAR 4 | DAR 3（不在 ACL） |
| GET 7000 A1–A4（类定义、实例未广告） | DAR 5 | DAR 3（7000 无属性在 ACL） |
| ACTION 7000 方法 127–129（A.8 定义、无处理器） | DAR 5 | DAR 5（ACL 广告） |
| ACTION 7000 方法 130（类/OI 未定义） | DAR 4 | DAR 3（不在 ACL） |
| ACTION 7010 方法 127–130（A.8 定义、无处理器） | DAR 5 | DAR 5（ACL 广告） |
| ACTION 7010 方法 131（未定义） | DAR 4 | DAR 3（不在 ACL） |
| GET/ACTION 未知 OI（0x7011、0x9999） | DAR 6 | DAR 3（ACL 先拒） |
| SET 7010 A2 | 无写处理器 | DAR 3（ACL 只读） |

没有任何路径返回空集合、空数组或伪造元素；没有方法返回成功。GET 走
`dcu_north_resolve_axdr` → `dcu_698_value_for_oad` → `_unadvertised_attr_rc`
（`dcu_698_oi_defines_attr`）；ACTION 走 `dcu_698_check_call` → `_class_defines_method`。

## 方法路由：根因与选择

根因：`_class_defines_method` 的类上限表把 Class 11 记为 `max_method=2`；127+ 的实例方法
只在有处理器的 OI（6000/6002/6012/6014 等）或 `k_oi_max_method` 的 A.12 条目里登记。
7000/7010 没有处理器，因此若不登记实例方法，`check_call` 会把标准已定义的 127–129/127–130
误判为类未定义（DAR 4）。

选择：沿用已有的 per-OI 127+ 方法表 `k_oi_max_method`，追加
`{OI_FILE_SET, 129}` 与 `{OI_SCRIPT_SET, 130}`。类 11 的类上限保持 2，其他
IC_COLLECTION OI 的 3..126 仍按类未定义回 DAR 4。

拒绝的替代：

1. 把 `k_class_max_method[11]` 抬到 130：会把方法 3..126 在所有 Class 11 OI 上广告成
   “标准已定义”，把本应 DAR 4 的请求变成 DAR 5，误伤 6002/6014/6016/6018/601A/601C/
   601E/6032/6034。
2. 给 7000/7010 加 stub 处理器并填 `method_mask`：会广告不存在的执行能力，且让
   `_class_defines_method` 的误判问题继续潜伏；主站可能把“已广告”理解为可实现。
3. 新建 `dcu_698_set.c`：没有处理器可落，单用途空文件不增加能力（与 D5-05/D5-06 先例一致）。

## 改动文件

- `cmd/dcu/dcu_router.h`：`OI_FILE_SET 0x7000`、`OI_SCRIPT_SET 0x7010`。
- `cmd/dcu/dcu_698_object_model.c`：两行对象；`k_oi_max_method` 两条；注释更新。
- `cmd/dcu/dcu_698_appconn.c`：`k_attrs_7010`、`k_methods_7000/7010` 与两条 ACL 行。
- `docs/design/DCU_698_OBJECT_MODEL.md`：计数、Class 11 已实现态行、§8.5 编码长度、§8.8。
- 测试：`tests/test_dcu_698_object_model.c`、`test_dcu_698_axdr.c`、`test_dcu_698_appconn.c`、
  `test_dcu_698_scheme_mgmt.c`、`test_driver_dlt698.c`。
- 未改 `dcu_698_axdr.c`：现有 fallthrough 已能对 GET/ACTION 给出 5/4/6，无需新分支。

## 计数与容量

- 对象表 103 → 105（`dcu_698_object_count()`）。
- Class 20 ACL 89 → 91（80 个手写行 + 11 个 `DCU_698_PARAM_OIS` 生成行）。
- 对象列表编码 91 项 = 3211 B（实测 `dcu_698_appconn_object_list()` 返回 len=3211；
  7000 行 29 B + 7010 行 41 B，89 项时为 3141 B）。
- ACL 会话存储 91 × 16 B = 1456 B + 表头 < `DRIVER_PRIV_SIZE 4096`；
  对象列表 3211 B < `EDGE_SLAVE_AXDR_MAX 10240`；105 < `EDGE_SLAVE_ACL_MAX_OBJECTS 224`。

## 与计划的偏差

1. 出口证据：计划写“GET 7000 返回集合元素”；A.8 的 7000 没有属性，本单改为
   “GET 7010 A2 DAR 5 + 7000 方法 127–129 DAR 5”。计划 §8 由 verifier 更新。
2. 交付物：计划写 `cmd/dcu/dcu_698_set.c`；本单没有新建该文件，因为没有可落的处理器。
   对象/ACL 行与 resolver 现有路径已足够表达诚实的 DAR 骨架。

## 验证

- 红：test commit `8146f199`（实现未落地时）目标 5 个测试可执行文件全部失败
  （`test_dcu_698_object_model` 计数 105≠103；`test_dcu_698_axdr` 7010 A2 期望 DAR 5 实际 6；
  `test_dcu_698_appconn` 计数 91≠89 且无 Class 11 ACL；`test_dcu_698_scheme_mgmt` 计数；
  `test_driver_dlt698` 计数与 DAR），见 `tests.log`。
- 绿：fix commit `c5adb633` 同一目标命令
  `ctest --test-dir build -R 'dcu|dlt698' --output-on-failure`：45/45 通过（首次绿 29.03 s；注释审计后 28.01 s）。
- 构建：增量构建 0 error；第一方 warning 0（仅第三方 open62541 既有 warning 不在增量内）。
- 守卫：`check_dcu_698_catalog.py` 0；`check_doc_references.py --root .` 0。
- 新用例：`test_class11_collection_rows_are_empty`、`test_product_class11_collection_sets_have_no_value_yet`、
  `test_class11_acl_is_read_only`、`test_slave_class11_collection_dar`、
  `test_object_list_advertises_new_ois`（计数更新）。
- 证据：`tests.log`、`runtime.log`、`diffstat.txt`、`comments.md`。

## 跟进项（不在本分支实现）

1. **集合存储 producer**：文件/脚本集合的元素类型、持久化与 D4-06 文件传输。有来源后把
   A2 加入对象 mask 与 resolver 编码，ACL 已就绪。
2. **计划状态更新**：verifier 按本单偏差改写 D5-03 出口证据，并把 `dcu_698_set.c` 交付物名
   改成对象/ACL 表的实际落点。
3. **方法定义表统一**：`_class_defines_method` 的 OI 特判（6000/6002/6012/8000）与
   `k_oi_max_method` 两套机制并存；若后续再增 OI 专属方法，考虑收敛为单一表（不在本单扩大）。

未合并，等 dcu-verifier 验证。
