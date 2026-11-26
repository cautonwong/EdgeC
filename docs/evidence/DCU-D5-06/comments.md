# DCU-D5-06 注释审计

范围：`git diff c2eaa5ea` 的代码、测试与设计文档；审查者 `pstack:comment-sicko`（report-only）。

## 删除

1. `tests/test_driver_dlt698.c` 的 `/* Class 17 显示无显示配置来源：F300/F301 只有对象行与只读 ACL。
   主站 GET 已广告属性由 resolver 回 DAR 5；ACL 未广告的属性先回 DAR 3；SET 回 DAR 3。 */`：
   复述下方断言（A2 DAR 5、A1/SET DAR 3），且无生产者理由已在
   `cmd/dcu/dcu_698_object_model.c` 的对象行注释；测试名与 `DLT698_DAR_*` 常量已承载契约。

## 保留

- `cmd/dcu/dcu_698_object_model.c` 的 F300/F301 行块注释：空掩码是有意为之（无显示配置 producer），
  “不存在”无法用代码表达；没有这条注释时空掩码与遗漏无法区分。
- `cmd/dcu/dcu_router.h` 的 `IC_DISPLAY 17` 行尾类名与 OI：承载 DL/T 698 的外部词汇
  （类 17 = F300 自动轮显 / F301 按键轮显），并沿用该头每类一注释的风格。
- `tests/test_driver_dlt698.c` 的 `/* A-Result Data 的 DAR 分支 */`：`payload[4] == 0x00`
  是表 182 的 CHOICE 线格式，断言本身看不出这个魔数含义；与本文件既有同款注释一致。

## 事实核对

- Class 17 与 `docs/698_parsed.md:4738-4801`（属性 1–4、方法 1–6）及 `:9313-9318`
  （F300 自动轮显、F301 按键轮显）一致。
- 文档计数与代码一致：对象表 103 行；ACL 89 = 78 手写 + 11 `DCU_698_PARAM_OIS`；
  对象列表实测 3141 B / 89 项。
- 对象行注释只点名 DAR 5；类未定义属性的 DAR 4 与 ACL/SET 的 DAR 3 由单测和设计文档行承载。

## 风险

- `docs/evidence/DCU-D5-06/runtime.log` 被 `.gitignore` 的 `*.log` 命中，提交时用 `git add -f`。
- 设计文档的 Class 17 行引用 `docs/evidence/DCU-D5-06/`；证据目录必须随修复提交一起入库，
  否则文档引用悬空。

## 行为核对

删除的只是测试注释，无行为变化；删除后 `test_driver_dlt698` 20/20 通过，目标套件 45/45 通过。
