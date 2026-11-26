# DCU-D1-10B 698 目录生成器按附录标题定位表格

基线 `4da3a938`（`feat/dcu` tip），分支 `dcu/D1-10B-line-window`，工作树
`/tmp/edgec-land/d1-10b-window`。未合并。

## 做了什么

关闭 `docs/plans/2026-11-21-dcu-698-production-plan.md` §12.5 的“生成器行窗口”
缺口：`scripts/extract_dcu_698_catalog.py` 的 `parse_ois`、`parse_oi_methods`、
`parse_oi_attributes` 都写死扫描 `docs/698_parsed.md` 的第 6298..9450 行。
原文抽取件一旦行号整体平移，窗口末尾的表行会被静默丢弃，生成器仍返回成功。

生成器现在先按附录标题定位：

- `appendix_a_window` 要求 `附 录 A` 与 `附 录 B` 标题各恰好一次且顺序正确，
  扫描范围取两者之间。
- 范围内要求对象表标题 `表A.1`..`表A.9`、`表A.11`..`表A.14` 各恰好一次；
  数据格式表 A.10 不属于对象表。标题缺失或重复直接报错退出。
- 每个对象表标题后 3 个非空行内必须有“对象名称”表头，否则报错。
- 三个解析函数改为接收该半开区间，仍用原文件下标，生成物里的 `source_line`
  保持绝对行号，正常输入下生成结果逐字节不变。

`tests/test_dcu_698_catalog_source_window.py` 新增 6 例：

1. 真实原文前插 140 个空行（把 A.14 的 F300/F301 推出旧窗口）后，生成目录与
   检入目录一致（忽略 `source_line`），生成夹具逐字节一致。
2. `附 录 A` 标题缺失 → 非零退出。
3. `附 录 A` 标题重复 → 非零退出。
4. `表A.7` 标题缺失 → 非零退出。
5. `表A.7` 标题重复 → 非零退出。
6. `表A.7` 表头缺“对象名称” → 非零退出。

`ci/scripts/run_guard_selftest.py` 的 CATALOG_GATE 段登记该测试。

## 根因

窗口是行号常量，与文档结构无关。平移 140 行后旧生成器退出码 0，写出
`26 classes, 512 OIs`，比检入目录少 `F300`、`F301` 两行；目录比较
`catalog_identical=False`。修复后同一输入写出 514 个 OI 且
`catalog_identical=True`。证据见 `runtime.log`。

## 验证

- 红：测试提交 `a8bbbe75` 在未修生成器上 6/6 失败，其中平移用例报
  `missing ['F300','F301']`，旧码退出码为 0（静默丢行）。
- 绿：修复提交 `768d7ed7` 后 6/6 通过。
- `python3 -I scripts/extract_dcu_698_catalog.py --check` 通过；非 `--check`
  重跑生成物后 `git status` 无变化，正常输入逐字节一致。
- `check_dcu_698_catalog.py`、`check_doc_references.py`、`run_guard_selftest.py`、
  `test_dcu_698_catalog`、`ctest -R 'dcu|catalog'` 见 `tests.log`。

## 未决

- 对象表编号集合（A.1..A.9、A.11..A.14）写为常量。若标准后续新增对象表，
  生成器会因“缺标题”报错，需要显式更新该集合；这是有意的失败方式。
