# DCU-D2-05a：/no-comments（Comment Sicko）

范围：`git diff 95824539..HEAD`，22 个文件（feature 提交 14 个；f7b506bf 时 18 个）。

## 第 1 轮（修复方，f7b506bf）

- 删除 27 处 / 28 行注释文本；保留协议线格式、公共 API 契约与故意畸形夹具说明。
- 落实 MUST KILL：capture_type 0/1/2 改为 `dcu_event_capture_t` 枚举。
- 拒绝并开放：op struct 按 method 拆 tagged union；`_encode_result` 的 `-2` sentinel（ASAN 修复轮已改为直接返回 typed rc）。
- 曾恢复 `dcu_scheme_store.h` 的“SQLite 权威、不在 RAM 复制”归属说明；第 2 轮删除，见下。

## 第 2 轮（独立 Comment Sicko，当前状态）

- 删除 9 处注释文本（8 条 + 1 阶段注）：
  - `dcu_scheme_store.h` 的归属/scope 说明。“没有执行者”是 D2-05b 的临时状态；DB 权威由 GET 直接读 SQLite 可见。
  - 枚举常量的 0/1/2 gloss。
  - 测试的四条复述：MS choice 3、等待/比对、128 upsert、非公历 DAR 7。
  - `result_append` 的“D2-05b 执行器”阶段注，保留 API/返回契约。
- 保留 13 组（有证明）：公共 decoder/encoder API 契约、OI 到 A.7/IC 的映射、octet-string 单字节长度、601B 单字节元素索引、`result_id` 1-based/0=min、singleton buffer/单线程 uv、ROAD 线格式、编号 0 保留、capture type 1 必须 NULL、6016 Clear 参数 NULL、date_time_s 向量。
- MUST KILL 处理：
  1. `dcu_scheme_store_transparent_apply` case 128：已抽 `_tp_upsert_content`（ASAN 修复轮）。
  2. `_encode_result`：已去掉 `-2` sentinel，直接返回 `EDGE_SLAVE_SRC_TYPE_MISMATCH` / `EDGE_SLAVE_SRC_TEMPORARY`（ASAN 修复轮）。
  3. `dcu_event_scheme_op_t` / `dcu_transparent_op_t` 按 method 拆 tagged union：仍开放，属形状变更。

## 其他

- 应用逻辑未因注释审查改变；枚举类型与结果容量上限是第 1 轮已声明变更。
- ASAN：修复后 `test_dcu_698_scheme_mgmt` 7/7，`detect_leaks=1` 无泄漏；`test_dcu_698_schemes`、`test_dcu_698_appconn`、`test_driver_dlt698` 同样 0 泄漏。
- 旧 comments.md 的 14 文件 / 28 处 / 38 行 / 恢复归属说明等描述已按实际 tree 修正。

结论：本轮删除 9 处注释；保留 13 组有证明注释；2 项 MUST KILL 已在 ASAN 修复轮落实，1 项开放；ASAN 0 泄漏。
