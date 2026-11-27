# DCU-P3-6-ZERO-SEQ-OI — 执行摘要

- 分支：`dcu/P3-6-zero-seq-oi`；worktree `/tmp/edgec-land/p3-6-zero-seq-oi`
- 基线：`feat/dcu` = `40d8dca5`（与 worktree HEAD 一致，期间未前进）
- 结论：**停在 spec**。标准可判定 OAD 是错误别名，但不存在可判定的零序电流 OI；
  一致修复需改 `cmd/dcu/dcu_698_rules.c`（移除种子行），超出本单文件边界。

## 标准结论

| 判定 | 依据 |
| --- | --- |
| OI 2002 属性 2 = 电压相角（Class 3，long-unsigned，度，-1） | `docs/698_parsed.md:6652`（表 A.3）、`:5752`（7.4.4 表 204）；`docs/698.pdf` pdftotext 行 6916；`DCU_698_OI_CATALOG.md:283` / `.json` OI 2002 |
| 标准无“零序电流”OI | `docs/698_parsed.md` 与 PDF 全文 `零序` 命中 0 次 |
| 最接近的量是零线电流，不是零序 | `docs/698_parsed.md:6651`：OI 2001 属性 4 `零线电流`（-3；IN=3I0） |
| 2027 电流不平衡率、3006 断流事件均非零序 | `:6709`、`:6906`（3006 为 Class 24 事件 OI） |

## 引用与主站行为

- 唯一生产引用：`cmd/dcu/dcu_698_rules.c:42` 种子规则行（DLT698）。
- 对象表/方案/ACL 无引用；北向名已删（`tests/test_dcu_north_source.c:91`）。
- 主站 GET/ACTION/Record 行为与本宏无直接路径；但规则种子行会影响 DLT698 采集映射，
  删除必须与宏同步（本单不做）。

## 本单交付

- `spec.md`：五栏 + 标准依据 + 引用清单 + 测试映射 + 停点。
- `tests/test_dcu_698_router_oad_catalog.py`：2 用例；当前 1 失败（`red.log`）。
- `catalog.log`：生成器 `--check` 与 `check_dcu_698_catalog.py` 均 rc=0；四个生成物
  sha256 前后一致（未改生成物）。
- 未交付（按停点）：commit 2 修复、变异守卫、`ctest -R 'dcu|dlt698'`、验收。原因：
  无 commit 2，守卫无被保护对象；验收要求的是修复后的主站行为不变。

## 未决（交 verifier 立项）

1. **跨文件修复单**：删除 `OAD_ZERO_SEQ_CURRENT` 宏 + `dcu_698_rules.c:42` 种子行 +
   更新 `tests/test_dcu_698_rules.c:67`（`test_dcu_698_scheme_mgmt.c:860` 换任意合法
   OAD）。只删宏会编译失败。删除后本测试转绿。
2. **变异守卫形态**：修复是“删除”，当前 TSV 单文件 find/replace 无法直接回退；
   需要 verifier 决定守卫写成“插回宏+行”的双文件守卫或改测试目标。
3. **相邻误标**（不在本单边界，未改）：
   - `cmd/dcu/dcu_soe_event_engine.h:24` 把事件 OI 3006 注释为“零序电流超限”；
     标准 3006 = 电能表断流事件。
   - `docs/design/DCU_698_OAD_STATUS.md:306` 把标准 OI 2002 写成“零序电流”。
   - `scripts/parse_exhaustive_698_dlms_idls.py:76`、`scripts/parse_full_pdf_schemas.py:49`、
     `scripts/build_exhaustive_idls.py:31` 硬编码 `0x20020200` 为零序电流。
   - `OAD_REACTIVE_ENERGY = 0x00200200` 在目录中为 OI 0020 反向有功电能（疑似命名错位，
     两协议侧都是反向有功），与本单根因不同。
4. **如果产品确需零序电流**：要么按 `IN/3` 从 OI 2001 A4 派生（需换算与命名约定），
   要么定义产品私有 OI（需目录/ACL/对象表同步）；两者都不能沿用 `0x20020200`。

## 裁决后实施（2026-11-27）

verifier 裁决最小删除后，本单已完成：
- 删 `cmd/dcu/dcu_router.h` 的 `OAD_ZERO_SEQ_CURRENT` 宏与 `cmd/dcu/dcu_698_rules.c` 种子行；
- `tests/test_dcu_698_rules.c` 期望表同步删除；`tests/test_dcu_698_scheme_mgmt.c` 的
  NOT_FOUND 场景改为对 `OAD_REACTIVE_POWER_TOTAL` 连删两次（首次 0、再次 NOT_FOUND）；
  `tests/test_dcu_north_source.c` 删零序断言；
- `docs/design/DCU_698_OAD_STATUS.md` 删该 OAD 行，并订正“OI 2002 = 电压相角 / 标准无零序 OI”；
- 新 pytest 保持为守卫并注册进 `ci/scripts/run_guard_selftest.py`（全绿）。
验证：pytest 删前红（red.log）、删后绿；`ctest -R 'dcu|dlt698'` 45/45；
`run_guard_selftest.py` 全绿；验收 1 轮退出 0、全部合格 2 个验收任务。
相邻误标（3006 注释、scripts 硬编码、OAD_REACTIVE_ENERGY 命名）按裁决留 follow-up。
