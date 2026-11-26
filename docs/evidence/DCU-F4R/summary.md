# DCU F4R：同 scheme_id 原地换 CSD 的执行身份

分支 `dcu/F4-scheme-identity`，合并 tip `60a13c85`（merge `168ab797`），基线 `feat/dcu 0066b073`。

## 问题与根因

执行身份 `dcu_exec_ident_t` 只有 `{exec_gen, scheme_id, item_mask}`。`dcu_poller_upsert_scheme` 允许在同一 `scheme_id` 下原地替换 `csd_list`；引用任务的执行身份不变，旧 `served` 位图按新 csd_list 的下标解释，造成新 tag 漏抄、旧 tag 重复读、表被记假成功。6014 store 对活动任务引用的方案拒绝改 CSD，但 poller 的 C API 本身不设防。

## 数据形状

- `dcu_scheme_t` 新增 `uint32_t content_gen`，由 poller 聚合根维护，调用方传 0。
- `dcu_poller_t` 新增 `scheme_gen_seq`；`add_scheme` 与 `upsert_scheme` 分配。
- `dcu_exec_ident_t` 增加 `scheme_gen`；执行身份 = `scheme_id/item_mask` + `scheme_gen` + `exec_gen`。
- `upsert_scheme` 比较 type、capture_mode、ms_type、storage_depth、storage_time、ms_addrs、ms_count、csd_count、csd_list；内容不同才递增代际并只重开引用该方案的 ACTIVE 任务；内容相同不重开。
- 重开递增 exec_gen，链上 `served_ident` 按新身份比较时清空旧 served；`scheme_gen` 是第二道保险。

拒绝的替代：把 6014 store 拒绝当唯一防线；重开全部 ACTIVE 任务；只比较 csd 内容而不入身份。

## 改动与提交

- `5195d2c8` 测试、`ba609c6f` 修复、`60a13c85` 证据。
- 文件：`cmd/dcu/dcu_poller.c/.h`、`tests/test_dcu_poller.c`、`docs/evidence/DCU-F4R/`。
- 新增 `test_scheme_content_change_reopens_referencing_execution`。

## 验证

- 红 1/35 → 绿 35/35；`ctest -R 'dcu|dlt698'` 45/45。
- coverage 验收 3 连绿，2/2 任务，35 passed，22.85/22.80/22.79s。
- 合并树守卫、全量 ctest、ASAN、fanalyzer、warning 在最终树上跑。

## 未决

- `dcu_poller_get_scheme` 可变指针 HOLD；`ms_count` 边界 HOLD。
- `dcu_scheme_store.c` 的 Set_CSD 直接改字段路径只对未引用方案开放，生产安全；彻底收口另立。
