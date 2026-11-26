# DCU-D5-05 Class 14/15/16 统计、平均、极值（DAR 诚实骨架）

分支 `dcu/D5-05-class14-16-stats`，工作树 `/tmp/edgec-land/d5-05-stats`，基线 `feat/dcu` `383c91cc`。

## 前提核查

标准表 163/166/169 给 Class 14/15/16 定义了属性 1（逻辑名）、2（结果表）、3（关联对象属性表）
与方法 1–4（复位、执行、添加、删除，全部可选）；目录 OI 2100–2104、2110–2114、2120–2124
分别属于 14/15/16（`docs/698_parsed.md:6732-6749`）。

产品没有统计生产者：`dcu_vertical_tasks`、`dcu_power_quality` 没有运行期实例，也没有
喂数、影子写入或存储行。广告任何统计值都会是假成功，因此本单只做 DAR 诚实骨架。

## 落地的数据形状

| OI | 接口类 | 范围 | 对象 mask | ACL |
| --- | --- | --- | --- | --- |
| 2100–2104 | IC_INTERVAL_STAT (14) | 终端级 | 空 | 属性 1..3 只读，方法 1..4 |
| 2110–2114 | IC_AVERAGE (15) | 终端级 | 空 | 属性 1..3 只读，方法 1..4 |
| 2120–2124 | IC_EXTREMUM (16) | 终端级 | 空 | 属性 1..3 只读，方法 1..4 |

对象 mask 为空，请求穿过驱动 ACL 后由对象表回答：

- 类定义、对象未声明的属性 → DAR 5（A1/A2/A3）
- 类未定义的属性 → DAR 4（A4 起）
- 类定义、对象未声明的方法 → DAR 5（方法 1–4）；类未定义的方法 → DAR 4（方法 5）
- 表外 OI → DAR 6（resolver 层）；主站请求先被 ACL 拒 → DAR 3
- ACL 属性权限全部是 1（只读）；SET 无写处理器 → DAR 3

没有 placeholder 值、空数组或返回成功的方法。Class 16 的类方法上限按表 171 从 2 修为 4
（`k_class_max_method`，此前方法 3/4 会被误判为类未定义）。

## 与计划原始出口的偏差

计划 §8 D5-05 的出口写“GET 2100、2110、2120 返回值”。本单没有数据源，按 D5-01/02 与
D4-02a 的先例改为 DAR 5 诚实骨架：GET 已定义但未广告的属性回 DAR 5，不返回伪造值。
计划状态与出口证据由 verifier 更新；数据源缺口见下。

## 计数与容量

- 对象表 86 → 101（`dcu_698_object_count()`）。
- Class 20 ACL 72 → 87（76 个手写行 + 11 个 `DCU_698_PARAM_OIS` 生成行）。
- 仍在 D1-21 的 `EDGE_SLAVE_ACL_MAX_OBJECTS 224` / `EDGE_SLAVE_AXDR_MAX 10240` /
  `DRIVER_PRIV_SIZE 4096` 以内；`_Static_assert(OBJECT_COUNT <= EDGE_SLAVE_ACL_MAX_OBJECTS)`
  继续把关。87 个 ACL 对象按 16 B/项计 1392 B（含表头仍远小于 4096）；对象列表编码
  2216 B（72 项）+ 15×53 B = 3011 B。

## 跟进项（不在本分支实现）

1. **统计 producer**：区间统计、累加平均、极值的运行期计算与存储；有来源后再把对应
   属性加入对象 mask 与 resolver 编码，ACL 已就绪。
2. **计划状态更新**：D5-05 的出口证据由 verifier 按本单偏差改写为 DAR 5 骨架。

## 验证

- 增量构建 92/92，第一方 0 warning（第三方 open62541 一条既有 unused-parameter 不计）。
- 同一命令 `ctest --test-dir build -R 'dcu|dlt698' --output-on-failure`：
  修复前 5/45 失败（exit 8，恰为本单 5 个测试文件）；修复后 45/45 通过（85.52 s）。
- `check_dcu_698_catalog.py`：0；`check_doc_references.py`：0。
- 证据：`tests.log`、`runtime.log`、`diffstat.txt`、`comments.md`。
