# DCU-D4-07 注释自审

范围：测试提交 `5dcbe1a3` 与修复提交 `d335de15` 的新增注释（未跑 comment-sicko；
按仓库注释纪律人工自评）。

## 保留

- `cmd/dcu/dcu_698_object_model.c` 的 F100 行注释：说明软件 ESAM 模拟只留 C API、698
  面空掩码不伪造序列号/会话/MAC，空掩码与遗漏无法用代码区分。
- `cmd/dcu/dcu_698_appconn.c` 的 `k_attrs_esam`/`k_methods_esam` 注释：15 属性只读、
  方法 1..8 广告、9..12 因每对象方法上限 8 记 HOLD；这是本单最容易被误读为遗漏的地方。
- `tests/test_dcu_advanced_modules.c` 的 698 不暴露软件 ESAM 注释：解释为什么方法 1
  现在回 DAR 5、并把“会话保持不变”作为断言依据。
- 测试里的 Class 21 / F101 说明：F101 是 Class 8 的目录事实无法从断言看出。

## 删除

- 删除 `cmd/dcu/dcu_698_class_security.c` 及声明/CMake 条目：698 面不再有 F100 处理器，
  保留会变成死代码与“可复位”的误导。
- 未改既有 why 注释。

## 行为核对

F100 空掩码 + 只读 ACL；软件 ESAM C API 不动；`ctest -R 'dcu|dlt698'` 45/45、六项守卫
0、mutation 1/1 敏感、验收连续 3 次 35/35。
