# DCU-P3-5-VAR-4510 交付摘要

分支 `dcu/P3-5-var-4510`，worktree `/tmp/edgec-land/p3-5-var-4510`，基线
`40d8dca5`（与 `feat/dcu` 相同，期间未前进，未 rebase）。未合并、未 push。

## 规格与口径

- 规格：`docs/evidence/DCU-P3-5-VAR-4510/spec.md`（五栏），设计：`design.md`。
- `dcu_value_t` 是标量结构：`type_tag` 集合无 octet-string(0x09)，
  `dcu_698_tag_width()` 只给标量宽度，`_encode_axdr_value()` 无 octet 编码。
  6 字节 MAC 不能进入标量端口，不得截断/塞进 `raw`。
- 4510 A5（`OI_ETHERNET_COMM_FIRST` + A5，OAD `0x45100500`）的唯一值源是 A-XDR
  resolver 的 `dcu_comm_info` MAC 快照。标量端口没有该属性的标量来源；按
  `docs/design/DCU_698_DEVIATIONS.md` Class 25/26 行“GET 无来源回 DAR 5”与规则 4，
  `dcu_698_value_for_oad(0x45100500)` 返回 `EDGE_SLAVE_SRC_CLASS_MISMATCH`
  （-21 → DAR 5），与 MAC 快照是否存在无关。修复前为
  `EDGE_SLAVE_SRC_TYPE_MISMATCH`（-22 → DAR 7）。
- 主站 GET 4510 A5 不变：`mac_present` 时 octet-string(6) + MAC 字节；缺失时 DAR 5。
  其它 Class 25/26 OAD 的标量 DAR 不变（类定义无来源 5、类未定义 4、未知 OI 6）。
- `DCU_698_DEVIATIONS.md` 未改：本单不改线上 DAR 口径或类行，内部标量 API 口径
  记录在本证据包；`check_doc_references.py --root .` 仍通过（rc=0）。

## 红先

- 测试提交 `aff418ba`（仅 `tests/test_dcu_698_var.c`；基线代码）。
- 命令：`cmake --build build -j1 --target test_dcu_698_var`（rc=0）；
  `ctest --test-dir build -R '^test_dcu_698_var$' --output-on-failure`。
- 原始失败：`[ ERROR ] --- -22 != -21`，
  `tests/test_dcu_698_var.c:334: error: Failure!`，9 个用例 8 过 1 败；同用例内主站
  GET 断言（octet-string(6) + `02 11 22 33 44 55`）在失败前已通过。原文 `red.log`。

## 绿后

- 修复提交 `c74f8ffd`（仅 `cmd/dcu/dcu_698_var.c`，+4/-1）。
- 定向：`ctest --test-dir build -R '^test_dcu_698_var$'` 1/1 通过；
  `ctest --test-dir build -R 'dcu|dlt698' --output-on-failure` **45/45 通过**，
  `Total Test time (real) = 129.67 sec`。原文 `tests.log`。

## 变异守卫

- `ci/mutation_guards.tsv` 追加
  `DCU-P3-5-VAR-4510-scalar-4510-a5`：把 A5 特殊返回改回旧行为，定向新用例。
- 命令：`python3 ci/scripts/run_mutation_guards.py --build build --only DCU-P3-5-VAR-4510-scalar-4510-a5`。
- 结果：mutated build rc=0 / test rc=8（红），restore 后 build rc=0 / test rc=0（绿），
  `1/1 guards sensitive`，退出 0。原文 `guard.log`。

## 验收（1 轮，串行锁）

- 命令：`flock /tmp/edgec-land/acceptance.lock env TESTPLATFORM=/home/edge/test-platform
  PYTHON=/home/edge/EdgeC/.venv/bin/python3 EDGEC_BUILD_DIR="$PWD/build"
  ./acceptance/run.sh dcu`。
- 结果：退出 0，`全部合格:2 个验收任务`；`dcu-acceptance` 3 passed（11.83 s），
  `dcu-northbound` 35 passed（27.51 s）。原文 `acceptance-1.log`。

## 门禁

- `python3 ci/scripts/check_doc_references.py --root .` → rc=0。
- `git diff --check` → rc=0。
- 未跑全量 ctest（批次约束）；构建配置 `ENABLE_WEB_BUILD=OFF`。

## 改动文件

- `cmd/dcu/dcu_698_var.c`：`dcu_698_value_for_oad` 的“已广告但无标量描述”分支，
  仅对 `OI_ETHERNET_COMM_FIRST` A5 返回 CLASS_MISMATCH。
- `tests/test_dcu_698_var.c`：新增
  `test_4510_a5_scalar_path_refuses_octet_string`（标量 DAR 5 × 有/无 MAC、
  主站 GET octet-string(6)、4510 A6/4500 A1/4511 A5 边界 DAR）。
- `ci/mutation_guards.tsv`：追加本单条目。
- `docs/evidence/DCU-P3-5-VAR-4510/`：spec/design/summary/red/tests/acceptance-1/
  guard/diffstat/comments。

## 未决

- 无口径歧义停点；标量端口不扩展 octet-string 是规格非目标。
- 未合并、未 push；`feat/dcu` 未前进，无需同步。
