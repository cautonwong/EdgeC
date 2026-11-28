# DCU-REFACTOR-01 独立验证报告

## 1. 独立预期推导
- **标准与架构依据**：`foundation/src/protocol/dlt698_codec.c` 的 `dlt698_decode_data` 与 `g_dlt698_item_pool[64]` 零动态分配契约。
- **独立期望值**：
  - 将递归 `ARRAY`/`STRUCTURE` 解码改为深度上限 `DLT698_DECODE_MAX_DEPTH = 8` 的显式栈迭代后：
    - 8 层以内的嵌套结构（`depth < 8`，总元素数 `<= 64`）应返回 `0` 并正确还原树形结构；
    - 9 层嵌套（第 9 个容器进入时 `depth == 8 >= DLT698_DECODE_MAX_DEPTH`）应返回 `-1`；
    - 单次调用内元素总数超过 `64`（环形池回绕后将覆盖 `call_pool_start`）应返回 `-1`，不得覆盖父层在用元素；
    - 所有标量与复合类型测试行为保持等价。

## 2. 合并树实测结果（`feat/dcu`）
- **Clean Release Build + Warnings**：`check_build_warnings.py` 0 warning。
- **目标回归**：`ctest --test-dir build --output-on-failure -R 'dcu|dlt698'` — **47/47 passed**（含 `test_dlt698_axdr` 8/8）。
- **ASAN**：`test_dlt698_axdr` — **1/1 passed**，0 泄漏/越界。
- **守卫自检**：`run_guard_selftest.py` — 全部通过。
- **结论**：`VERIFIED`。
