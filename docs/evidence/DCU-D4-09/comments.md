# DCU-D4-09 注释自审

范围：测试提交 `d50e3395` 与修复提交 `009e8fa1` 的新增注释（未跑 comment-sicko；
按仓库注释纪律人工自评）。

## 保留

- `cmd/dcu/dcu_698_object_model.c` 的 2301–2308 块注释：产品无总加配置/总加值/滑差/
  功控来源，空掩码让 resolver 回 DAR 5，总加与功控方法没有处理器绝不执行；“不存在”
  无法用代码表达。
- `cmd/dcu/dcu_698_appconn.c` 的 `k_attrs_group` 注释：A1..A18 的类定义、ACL 属性
  schema 只到 16，A17/A18 记 HOLD；这是本单最容易被误读为遗漏的地方。
- 测试中的表190 / 方法 1..5 说明：标准出处与“无处理器”事实断言本身看不出。
- `DCU_698_DEVIATIONS.md` 的 Class 23 行：producer 缺口与 ACL schema 限制的现行口径。

## 删除

0 条。未改既有 why 注释。

## 行为核对

8 个对象行与只读 ACL；未改 resolver/驱动/编码器。`ctest -R 'dcu|dlt698'` 45/45、
六项守卫 0、mutation 1/1 敏感、验收连续 3 次 35/35。
