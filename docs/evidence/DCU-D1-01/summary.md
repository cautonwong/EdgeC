# DCU-D1-01 — 698 catalog class reconciliation

- Base: main `c949155c`
- Branch: `dcu/D1-01-catalog-classes`
- Date: 2026-11-22

## Result

79 rows had `parsed_class != inferred_class`. Each was checked against its
Appendix A row and `docs/698.pdf`:

- **75 resolved** to the class the standard assigns. For 65 of them the printed
  IC was already right and the range heuristic was wrong; 10 are OCR
  corrections where the printed IC is a misread (201E..202C print 8, 4022
  prints 6).
- **4 deferred**: the standard class differs from the class the product object
  table uses. Changing them here would change product behaviour, so the catalog
  records the standard class and keeps the product class until a separate
  defect updates the table:
  | OI | standard | product |
  | --- | --- | --- |
  | 6000 采集档案配置表 | 11 (集合) | 10 (采集监控) |
  | 6002 搜表 | 11 | 10 |
  | 6003 搜表结果表 | 8 (参变量) | 10 |
  | 8000 远程控制 | 8 (参变量) | 13 (负荷控制) |
- **0 unexplained** after the change.

The 201E..202C decision is the only place the printed IC is overridden on OCR
grounds: 201C and 2026..202E print 6 and 7.3.6 defines those values as data
variables. 4022 prints 6 between 4021/4024 which print 8.

## Gate

`scripts/extract_dcu_698_catalog.py` now owns `OI_CLASS_DECISIONS`
(`parsed`, `model`, `source`, `deferred`). The generator fails — for both
generation and `--check` — when:

- a conflicting row has no decision, or
- a decision's recorded `parsed` no longer matches the printed IC (a re-OCR), or
- a decision's `model` is not what the row uses, or
- a decision names an OI absent from the parsed table.

`run_guard_selftest.py` adds a refutation fixture: mutate a decided row's IC and
the catalog gate must reject it. Probes in `runtime.log` cover decided,
deferred, OCR-corrected, and undecided rows; all are rejected.

The reconciliation table is generated (`docs/design/DCU_698_CLASS_RECONCILIATION.md`)
and part of the `--check` comparison, so code and document cannot drift.

## Not changed

No object-table masks, class ids, or product behaviour. The D1-10 catalog test
still matches the fixture because the four deferred rows keep their product
classes (10/13).

## Gates

Clean build (0 new warnings), 30/30 dcu+dlt698, catalog gate, guard selftest,
acceptance 3 + 9, `git diff --check`. See `tests.log`.
