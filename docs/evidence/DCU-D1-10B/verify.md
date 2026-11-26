# DCU D1-10B 验证记录（dcu-verifier，2026-11-26）

分支 `dcu/D1-10B-line-window`，merge `8127a473`（tip `24d8f39b`），基线 `4da3a938`。

## 结论

VERIFIED。目录生成器改为按 `附 录 A` / `附 录 B` 标题定位附录 A，再按 `表A.1`..`表A.14` 标题与“对象名称”表头定位各表；缺标题或重复标题直接非零退出，不再依赖 6298..9450 固定行窗口。真实输入下三份生成物与基线逐字节一致（`git diff 4da3a938 -- docs/design tests/fixtures` 为空）。

## 红先绿后（独立复跑）

- 红：在 `docs/698_parsed.md` 前插入 140 行后，旧生成器静默写 512 OI（漏 F300/F301），新生成器写 514 OI 且与真实目录一致；测试 `tests/test_dcu_698_catalog_source_window.py` 6 例在旧窗口逻辑上红。
- 绿：tip `24d8f39b` 同套件 6/6 OK；变异守卫 `DCU-D1-10B-fixed-line-window`（把标题定位改回固定窗口）独立复跑 red rc=1 → green，1/1 sensitive。

## 合并树门禁

- `extract_dcu_698_catalog.py --check`、`check_dcu_698_catalog.py --root .`、`check_doc_references.py` 0；`test_dcu_698_catalog` 5/5。
- 并入后与 D4-04 同批跑：合并树 build 0 第一方 warning、守卫自检全绿、全量 ctest 367/367、变异守卫 `DCU-D1-10B-fixed-line-window` 1/1 sensitive；原文在 `docs/evidence/DCU-D4-04/verifier/merged-*.log` 与 `verifier/`。
- 无产品 C 代码改动，未跑 ASAN/fanalyzer/验收。

## 残留

- 期望的对象表集合是显式常量：标准新增对象表时生成器会响亮失败并要求更新清单，属有意设计。
