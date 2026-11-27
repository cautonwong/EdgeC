# DCU-P3-9-RUNTIME-METHODS design（短）

## 结论

运行时类方法定义从“1..max 全放行”改为“1..max 减去 7.3 缺号”。缺号只在
Class 7、9、24 出现，故用既有上限表加一列位图表达，不重排表：

- `k_class_max_method` 增加 `absent_methods`：bit n = 方法 n 不在 7.3 类表。
  Class 7 → bit 3、Class 9 → bit 6、Class 24 → bit 3；其余类为 0。
- 新增公开谓词 `dcu_698_class_defines_method(class_id, method_id)`：方法号非 0、
  在类上限内且不在缺号位图中才为 true。与既有 `dcu_698_class_defines_attr` 对称，
  让无运行时实例的 Class 7 也可被测试与 resolver 复用。
- `_class_defines_method(class_id, oi, method_id)` 保留 OI 专属分支（127+、F001
  9/10、8000 127–131 等）在前，末尾改调公开谓词；删除 Class 9/方法 6 的一次性特判
  （并入位图，单一来源）。

## 为什么不改 ACL

`dcu_698_appconn.c` 的广告是逐 OI 手写表，本就只列真实方法：5000–5007 广告
`{3,4,5,7,8}`，3000/3005 无方法，Class 7 无对象。缺号方法从不进入 Class 20 列表，
主站 ACTION 在 ACL 层先回 DAR 3；本单只修正 ACL 之后 resolver 的 DAR 4/5 判定，
因此 appconn 零 diff。

## DAR 分层（不变）

未广告 → 驱动 ACL DAR 3；类/方法未定义 → `EDGE_SLAVE_SRC_UNDEFINED` DAR 4；
类定义但无来源/未掩码 → `EDGE_SLAVE_SRC_CLASS_MISMATCH` DAR 5；未知 OI →
`EDGE_SLAVE_SRC_NOT_FOUND` DAR 6。

## 验证

- 红：Class 24 方法 3 当前回 DAR 5；修复后 DAR 4。
- 绿：定向 ctest（dcu|dlt698）。
- 守卫：变异把谓词改回 max 放行 → `test_class_method_sets_exclude_standard_holes` 红。
- 验收：`acceptance/run.sh dcu` 2 任务。
