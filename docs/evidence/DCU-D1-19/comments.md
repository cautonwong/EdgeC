# DCU-D1-19 注释审计

范围：`e5906b65` 到工作树的 diff，新增的四个类族文件、`dcu_698_object_internal.h` 与被裁剪的 `dcu_698_object_model.c`。

## 结果

删除 0 处；新增代码里没有死注释、复述代码或历史叙述。

## 保留

- `dcu_698_class_acq.c` 的 `[Legacy/Testing mapping] 127/128`：搬移后仍成立（mask 位与分支都在）。
- `dcu_698_class_acq.c` 的 Class 9 方法 3/4/5/7/8 说明：`-4` 语义未变。
- `dcu_698_class_event.c` 的 7.3.24 表193 映射：协议规则。
- `dcu_698_class_control.c` 的 `1=跳闸 0=合闸允许`：非显然的动作编码。
- `dcu_698_object_internal.h` 的 “类族处理器只由对象表与快照分派引用”：所有权约束，声明本身看不出。

## 行为核对

处理器逐字搬移；共享状态经访问器读取，返回值与原静态变量相同。唯一其他改动是删除 `dcu_698_snapshot()` 里永远不进入的空分支 `else if (obj->class_id == IC_RECORD) {}`，无行为变化。
