# DCU-D1-10 注释审查

工具：`pstack:no-comments`（comment-sicko 子代理，opus），范围为本单未提交的改动。

## 接受的处理

删除 1 处：`tests/test_dcu_698_catalog.c` 的 `@file`/`@brief` 文件头。它重复文件名与测试目的；测试二进制没有公开 API 契约，无保留条件。

## 保留的注释

- `cmd/dcu/dcu_698_object_model.h` 两个新导出函数的 `@brief`：第二个写明越界返回 NULL，签名本身表达不了；该头文件所有导出函数都按这个方式写。
- `scripts/extract_dcu_698_catalog.py` 中 `parse_oi_methods` 的 docstring：7.3 类表不含附录 A 的按 OI 方法，这是标准的结构划分，审查人核对了 `docs/698_parsed.md` 的行号分布。

## MUST KILL（记录，未改）

`parse_oi_methods` 与既有 `parse_ois` 一样用硬编码行窗口 `6298..9450`。审查指出 9450 已经越过附录 A（附录 B 约从 9330 开始，最后一条方法行在 9282），建议改为从附录标题推导边界。这是既有代码的模式，本单只是沿用；改成推导边界属重构，另立任务。

## 审查后的验证

- `test_dcu_698_catalog` 重新编译并运行：3/3，退出 0。
- 其余门禁不受影响（仅删测试文件头注释）。
