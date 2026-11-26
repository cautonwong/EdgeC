# DCU-D1-10A 注释审查

工具：`pstack:comment-sicko`（只读子代理），范围为本单未提交的改动。
结论：5 条 KEEP，1 条 MUST KILL，已按 MUST KILL 删除 1 行；未删除任何既有注释。

## 接受并执行的删除

- `tests/test_dcu_698_catalog.c` 的内联注释
  “7.3 类表方法对同类 OI 通用；附录 A 的号只属于列它的 OI。”：
  两行 `continue`/`assert_true` 已表达同一规则；夹具表头
  `dcu698_catalog_oi_methods[]` 与产品 `dcu_698_object_model.c` 的
  `_class_defines_method` 注释已各自说明来源。删除后无执行约束丢失。

## 保留

- `scripts/extract_dcu_698_catalog.py` 生成的夹具表头：说明 7.3 类表方法与
  附录 A 按 OI 方法的划分；裸 `{oi, method}` 行看不出越界号只属于该 OI。
- 空表哨兵行 `{0x0000, 0}, /* 无 OI 专属方法 */`：与既有属性表哨兵同形，
  生成器无方法时仍保持 C 数组非空。
- `dcu698_catalog_class_t` 的 `max_method` 行内注释：说明它是对 7.3 与该类
  OI 附录 A 方法的按类最大值，是派生值，新测试把它当阈值读。
- `max_class_method` 行内注释：与上一个上限相邻，必须写明它不含 OI 方法。
- `docs/design/DCU_698_OI_CATALOG.md` 新增小节的说明：面向读者说明“7.3 已对
  同类全部 OI 定义的方法号不看本表，其余号只对列出的 OI 有效”。

## 未覆盖（审查员记录，本单已界定）

新测试用 `max_class_method` 作为类方法代理，所以 7.3 类表内部跳号
（Class 7/24 的方法 3、Class 9 的方法 6）仍会放行。这是既有类级 `max_method`
检查的行为；本单只收口附录 A 方法跨 OI 泄漏，summary 的“发现与未覆盖”已记录。
未来若对象广告这些跳号，需另立任务改成精确类方法集合。

## 审查后的验证

- `test_dcu_698_catalog` 重新编译并运行：5/5，退出 0。
- 变异 `DCU-D1-10A-wrong-oi-method`：失败行随注释删除从 82 变为 81；先红
  `test rc=8`，恢复后绿 `test rc=0`。
