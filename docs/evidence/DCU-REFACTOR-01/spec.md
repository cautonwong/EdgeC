# DCU-REFACTOR-01 五栏规格：`dlt698_decode_data` 显式有界栈迭代重构

## 1. 业务规则
1. 将 `foundation/src/protocol/dlt698_codec.c` 中的 `dlt698_decode_data` 从递归调用改为显式有界栈迭代实现，消除 `ARRAY` / `STRUCTURE` 嵌套在 CBMC 5.12 下的 `O(N^N)` 符号展开爆炸与运行时深嵌套栈溢出风险。
2. 定义显式最大嵌套深度常量 `DLT698_DECODE_MAX_DEPTH`（例如 `8` 层，足以覆盖 DL/T 698.45 方案/CSD/RCSD/Record 数据结构；超过最大深度时返回 `-1` 拒绝，不得越界或回绕）。
3. 保持 `g_dlt698_item_pool[64]` 零动态内存分配语义、所有已支持 tag 的解码行为与返回码（`0` 成功、`-1` 截断/越界/超限、`-2` 不支持 tag）与重构前完全一致。

## 2. 不变量
1. **无递归调用**：`dlt698_decode_data` 函数体及辅助函数不得直接或间接递归调用自身。
2. **有界栈与池不变量**：显式栈深度 `sp < DLT698_DECODE_MAX_DEPTH`；`g_dlt698_item_pool[64]` 索引不越界；无 `malloc`/`free` 等动态内存分配。
3. **行为等价**：所有现有 `test_dlt698_*` 与 `test_dcu_*` 单元测试保持全绿。

## 3. 可观测输出
- 对嵌套深度 `<= DLT698_DECODE_MAX_DEPTH` 且总元素数 `<= 64` 的 `ARRAY`/`STRUCTURE` 报文，`dlt698_decode_data` 正确填充 `dlt698_data_t` 树并返回 `0`。
- 对嵌套深度超过 `DLT698_DECODE_MAX_DEPTH` 的报文，返回 `-1` 且不写越界内存。
- 对现有全部标量与复合类型测试，输出逐字节/逐字段不变。

## 4. 验收判据
1. `tests/test_dlt698_axdr.c` 新增嵌套边界测试（合法多层嵌套通过 + 超过 `DLT698_DECODE_MAX_DEPTH` 返回 `-1`）。
2. Clean Release build 0 warning，`ctest --output-on-failure -R 'dcu|dlt698'` 全绿。
3. ASAN 下 `test_dlt698_axdr` 无栈溢出、无越界、无泄漏。

## 5. 非目标
- 不在本单编写 CBMC harness（留待重构合入后的 `DCU-FORMAL-07`）。
- 不扩大 `g_dlt698_item_pool[64]` 容量或改变 `dlt698_data_t` 公开结构体布局。
