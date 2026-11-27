# 设计（短）

## 测试形态

`tests/test_dcu_698_router_oad_catalog.py`，unittest，直接读仓库源文件，无需构建：

1. 从 `cmd/dcu/dcu_router.h` 提取所有 `#define OAD_* 0x...`（先合并 `\` 续行），
   OAD 的 OI = 值高 16 位。
2. 从 `docs/design/DCU_698_OI_CATALOG.json`（由 `docs/698_parsed.md` 生成）建 OI 表。
3. 用例 A：每个 `OAD_*` 的 OI 必须在目录存在（当前绿）。
4. 用例 B：`OAD_ZERO_SEQ_CURRENT` 不存在视为合规；存在则其 OI 的目录名必须含
   “零序”。当前红：名称为“电压相角”。

用例 B 以“缺失即合规”收口，是因为标准没有零序 OI；后续若按 R4 移除宏与种子行，
该用例转绿；若未来标准/产品定义了零序 OI，再以真实 OI 断言。

## 为什么不做全量语义对拍

全量 OAD 语义对拍会把相邻的既有命名问题一起判红：`OAD_REACTIVE_ENERGY = 0x00200200`
在目录里是 OI 0020 反向有功电能（DL/T645 侧 0x00020000 也是反向有功），属命名错位而非
映射错位，与本单根因不同。本单只钉住零序 OAD，相邻问题记入 summary 的未决，避免把
无关缺陷混进同一条变异守卫。

## 为什么停在 spec

标准可判定 `0x2002 A2 = 电压相角`，但不可判定零序电流 OI；保持主站行为不变的唯一
处置是删除宏与 `cmd/dcu/dcu_698_rules.c:42` 种子行，而该文件不在本单文件边界内。
按任务约束停在 spec，不实施 commit 2。
