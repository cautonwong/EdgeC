# DCU-D2-02 注释审计

范围：`f0ec96fe` 到工作树的 diff，含新增的 `tests/test_dcu_698_archive.c` 与勘察文档。

## 删除

15 行注释、9 处：6 行纯函数名复述（`dcu_inventory.h` 的三个 `@brief`）、2 行叙述（旧调用方保留驱动名、字段完整）、2 行复述断言、2 个章节横幅（`Class 10 / OI 6000`、`OI 6000 方法参数`），以及 `/* 档案已存在 */`。

## 保留

- A.7 参数布局：6001 A2 结构、129/130/133 参数结构、Basic 字段表（波特率/规约 enum、端口 OAD 末字节、密码/资产号 octet-string、换算 -1）。
- DAR 规则：类定义未广告 → 5、未定义 → 4；批量上限 8；8000 129–131 的 DAR 2。
- 存储约束：单 SQLite 连接、ALTER TABLE 增量列、config_seq 补号、附属信息无存储路径、批量全成或全不成、落库失败不回 RAM。
- 非显然不变量：config_seq 0 = 未分配、A-XDR 数组 255 项/2048 字节上限。

## 说明

- 审计期间工作树有并发编辑（north_adapter 重构为 `dcu_698_accept_archive_action`、设计文档更新）；最终状态已复读。
- 注释改动后重跑了构建、全量 ctest、ASAN、守卫与验收。
