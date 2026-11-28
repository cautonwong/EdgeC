# DCU-D6-01b 独立验证 (`verify.md`)

规格：[spec.md](spec.md)（含 2026-11-28 verifier 订正：4000 A3 走标量通道）。基线 `bc777aaa`，worker HEAD `13338ddf`，合并 `31a39e95`。

## 独立推导

从标准原文推导期望，不读实现输出：

- `docs/698_parsed.md:7710`：4000（Class 8）定义属性 2、3、4 → 4000 A3/A4 是标准已定义；A5 未定义。
- `docs/698_parsed.md:8756`：8001（Class 8）定义属性 2，随后 8758–8762 给出属性 3、4、5 → 8001 A3..A5 已定义；A6 未定义。
- 7.3.8 参数变量接口类（第 4124 行附近）类表只到属性 2，所以这两个 OI 的 A3+ 必须靠实例上限表登记才不会被误判为“类未定义”。
- 变量类 Class 1 定义属性 1..5 → 0010 A1 已定义未实现应是 DAR 5；A6 类外应是 DAR 4；不在对象表的 OI 仍是 DAR 6。

## 观测

| 期望（独立推导） | 观测命令 | 结果 |
| --- | --- | --- |
| 4000 A3/A4、8001 A3..A5 定义；4000 A5、8001 A6 未定义 | `CMOCKA_TEST_FILTER=test_discovery_oi_attributes_beyond_class_table ctest -R '^test_dcu_698_object_model$'` | PASS |
| 0010 A1 → DAR 5、A6 → DAR 4、9999 → DAR 6 | `CMOCKA_TEST_FILTER=test_variable_attr_dar_classification ctest -R '^test_dcu_698_var$'` | PASS |
| 4000 A3 标量通道 → DAR 5 | `test_driver_get_unadvertised_and_undefined_dar`（合并树 `test_dcu_698_axdr`） | PASS |

独立探针（单独编译 `dcu_698_oi_defines_attr`）因静态库链接依赖过多未跑通，已放弃；分类期望的推导来自标准原文，观测来自合并树的定向用例。

## 结论

**VERIFIED**（定义表与 DAR 分类）。未覆盖：8001 A2..A5 仍在对象 mask 中走 DAR 2 的现有路径，本单只修分类表，不改值通道。
