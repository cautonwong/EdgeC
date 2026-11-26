# DCU-D4-08a：/no-comments（Comment Sicko）

范围：`git diff 6ad00780..HEAD` 加未跟踪 `docs/evidence/DCU-D4-08a/*`，15 个文件。

## 接受的处理

- `cmd/dcu/dcu_698_object_model.c`：Class 22 来源说明从 2 行改为 1 行精确说明。删除
  误导性的 inventory 波特率括号（inventory 是表计侧参数，不是端口 COMDCB），也不再
  声称所有 ACL 属性都回 DAR 2（F200 A3/A4 由掩码挡住，回 DAR 5）。
- `tests/test_dcu_698_class22.c`：删除文件头第二行（“不伪造数据”是断言已表达的意图，
  属说教）；保留 A.12 范围事实（解释循环上界）。
- `tests/test_dcu_698_axdr.c`：F200 A2 注释收窄为 why（已广告但只读 → SET DAR 3），
  删除本测试未覆盖的 “GET 回 DAR 2” 半句。
- `acceptance/dcu/tests/test_dcu_northbound.py`：删除新用例 docstring；`clause=` 已
  表达同一合同，docstring 只是重复。

## 接受的代码整改（report 的 reshape flags）

- `cmd/dcu/dcu_698_var.c`：删除 `{OI_CHANNEL_CONFIG, 3, VK_U8}`。F200 A3 不在对象掩码，
  该标量描述与 A2 一样不可达（`dcu_698_value_for_oad` 先被掩码挡住），是遗留的假
  校验位标量，已与 A2 一起清除。
- `cmd/dcu/dcu_698_axdr.c`：Class 22 分支条件从 `oi >= 0xF200 && oi <= 0xF20C` 改为
  `per->class_id == IC_PERIPHERAL`。范围不再在 resolver 里第三次手写，未来新增
  IC_PERIPHERAL 行不会漏掉 resolver。

## 拒绝的报告项

- `dcu_router.h` 10 条宏尾注（RS485 端口/红外端口/…）：与相邻既有 OI 宏注释同风格
  （接口类名与标准名），删除会破坏该宏块的一致性；F200/F209/F20C 三条标准名是keep，
  已保留。
- “用一张 Class 22 描述表同时驱动 ACL 与对象掩码”：跨 ACL 与对象表的结构重构；F200
  A3/A4 是有记录的既有偏差（ACL 可写、对象掩码未广告 → DAR 5，验收 000014 锁定）。
  不在本单，见遗留。

## 保留（有证明）

- `tests/test_dcu_698_value_table.c` 的 “F200 A2 是端口配置数组（COMDCB），标量端口
  明确拒绝”：解释 TYPE_MISMATCH 与 CLASS_MISMATCH 的区别，与相邻 5004 注释同风格。
- `tests/test_dcu_698_class22.c` 的 “附录 A.12：F200..F20C 全部属于接口类 22（F210 是
  类 8）”：核对 `docs/698_parsed.md`，解释循环上界。

## 遗留

- ACL 与对象掩码没有逐项交叉校验（现有设计文档 §8.5 已记录）。F200 A3 的 ACL 读权限
  与 A2 掩码不一致；是否让 A3 服务需要先有来源，另立任务。
