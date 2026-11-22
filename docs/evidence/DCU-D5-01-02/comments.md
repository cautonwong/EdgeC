# DCU-D5-01/02 注释审计

范围：`git diff a3f8d40f` 的 10 个受控文件与 `docs/evidence/DCU-D5-01-02/`；`.audit/` 忽略。审查者 `pstack:comment-sicko`。

## 删除

1. `cmd/dcu/dcu_router.h` 的 `IC_MAX_DEMAND` 行尾注释：符号名已说明含义，`(1010 家族)` 是对象表 OI→类映射的第二份拷贝。
2. `cmd/dcu/dcu_router.h` 的 `IC_HARMONIC` 行尾注释：同上，`(200D/200E)` 由对象表拥有。

## 保留

- 设计文档的 Class 2/5 状态段属于设计状态同步，不在代码注释范围。
- `cmd/dcu/dcu_router.h` 其余 12 条 `IC_*` 行尾注释是既有模式，不在本次 diff 内，未扩大范围。

## 风险

- 六个空掩码对象行从代码上看起来像遗漏；意图由设计文档与 `test_product_class2_and_5_have_no_value_yet` 承载，未加解释性注释（避免为一次性结构加文字）。
- `runtime.log` 被 `.gitignore` 的 `*.log` 命中，提交时用 `git add -f` 保留证据。

## 行为核对

删除注释后重新构建、`test_dcu_698_axdr` 通过；无行为变化。
