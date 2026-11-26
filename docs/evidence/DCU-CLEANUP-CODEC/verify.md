# DCU CLEANUP-CODEC 验证记录（dcu-verifier，2026-11-26）

分支 `dcu/cleanup-codec-tags`，merge `a106bec7`（tip `7f3bcf71`，refactor `69dc71bb`），基线 `8236eb3d`。

## 结论

VERIFIED（行为等价重构）。`dlt698_codec.c` 中 9 处已有 `DLT698_TAG_*` 常量的 A-XDR 标签字面量（`18`/`5`/`28`、`0x1C`/`0x00`/`0x54`）改为常量；`0x19`（date_time）无对应常量，保留并原位注明。无编码、CHOICE、控制码、DAR 或测试改动。

## 等价性证据（独立复跑）

- 独立重建 `edge_codec` 对象：基线 `8236eb3d` 与 tip `7f3bcf71` 的 `dlt698_codec.c.o` sha256 完全相同（`eaa6b7c4…94dfc`），原文 `verifier/base.sha`、`verifier/tip.sha`。
- `ctest -R 'dlt698|driver_dlt698'` tip 7/7；diff 仅 9 行替换 + 1 行说明注释，原文 `verifier/tip-ctest.log`、`verifier/diff.txt`。

## 合并树门禁（`a106bec7`）

- build/warnings/守卫自检/全量 ctest 见 `verifier/merged-*.log`；fanalyzer 与 D4-09 合并树同批跑（常量替换不改变对象码）。
- 无行为改动，未跑验收/ASAN。

## 残留

- `0x19`（date_time）待 `foundation` 头新增常量后再换。
