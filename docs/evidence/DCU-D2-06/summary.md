# DCU-D2-06：601C/601D 上报方案（管理平面）

## 执行面停止报告（先评估，未扩权）

D2-06 计划行要求“配置通道、超时、重试与数据列表，接 SnF”，出口证据是“模拟主站断开后恢复，数据按序补齐”。本树不具备执行前提：

- ADR-0017（`docs/adr/0017-698-slave-report-port.md` §3/§4）明确把“未确认重发、达到次数放弃、存储转发（SnF）”留在本期之外，需要跨请求重发状态与持久化，“另立任务”；生产来源（事件入队）也不接，等 D1-07 记录取数与 D3-05b。
- `REPORT-Notification` 只实现 `CHOICE [1]` 对象属性列表；`CHOICE [2]` 记录型上报返回 `DLT698_REPORT_UNSUPPORTED_RECORD`（`foundation/include/dlt698_codec.h`）。601D 上报类型 1（RecordData）需要它。
- D3-05b 事件上报接线未合并（plan §12.5 下一批第 2 项）。
- 现有 `dcu_698_report` 队列是固定 32 条、best-effort、满了丢最旧；没有落库、没有按序补齐、没有重试计数。

结论：SnF/重试执行需要改 ADR-0017、驱动重发状态与持久化队列，属 plan §9.5 #5（公共组件/ADR）并依赖 D3-05b（#1）。按 dcu-verifier 指示，本单只做管理平面，不假装执行，也不新增第二套队列；验收“主站断开恢复、数据按序补齐”**不在本单**，归 D2-06b/D3-05b。

## 数据形状

601C/601D 与 D2-05a 的三个族共用既有 `dcu_scheme_store`（同一 SQLite 连接、事务/回滚风格）。没有执行者，SQLite 是权威，GET 直接查询，不在 RAM 复制，也不复用/改造 `dcu_698_report` 队列。

- 键：方案编号 unsigned，1…255；0 保留（OAD 元素 0 = 最小现有编号）。
- 601D 单元：`{编号, 上报通道 array OAD, 上报响应超时时间 TI, 最大上报次数 unsigned, 上报内容 structure{类型 unsigned, 数据 Data}}`。
- 上报通道：1…4 个 OAD（产品上限）；0 个视为无效配置，Add 回 DAR 2。
- TI：`0x54 | unit(1) | value_be16`；unit 只接受 0…3，越界 DAR 7。
- 最大上报次数：unsigned，原样保存（执行归 D2-06b）。
- 上报内容：类型 0 = OAD（对象属性数据），保存 OAD；类型 1 = RecordData（OAD + RCSD + RSD）标准合法但产品没有存储/执行路径，按 DAR 2 拒绝（能力诚实，不解析 RCSD/RSD 也不静默存半截）；类型 >1 回 DAR 7。
- 方法：127 Add（array 601D）、128 Delete（array 编号）、129 Clear（NULL）。缺编号回 DAR 6，整批全成全败。
- 601D A2 的元素索引 = 方案编号（0 = 最小现有）；不存在回 DAR 6。

## 对象与 ACL

| OI | 类 | 属性 | 方法 |
| --- | --- | --- | --- |
| 601C | 11 集合 | A2 只读 | 127/128/129 |
| 601D | 8 参数 | A2 只读 | — |

对象列表新增 2 项，总数 42 → 44；ACL 上限当前 44，不再调整。

## 持久化

同一 `storage->db` 新增：

- `dcu_report_scheme(scheme_id PK, timeout_unit, timeout_value, max_times, content_type, content_oad)`
- `dcu_report_channel(scheme_id, pos, oad, PK(scheme_id,pos))`

写入走 `BEGIN IMMEDIATE` 事务，失败回滚并回 DAR 2；按编号升序读取。

## 实现落点

| 关注点 | 文件 |
| --- | --- |
| 类型与对外 API | `cmd/dcu/dcu_scheme_store.h`（`dcu_report_scheme_cfg_t` / `dcu_report_op_t` / 5 个函数） |
| 持久化与校验 | `cmd/dcu/dcu_report_scheme_store.c`（233 行；从 `dcu_scheme_store.c` 拆出以满足 SRP 1500 行） |
| 共用连接与事务 | `cmd/dcu/dcu_scheme_store_internal.h`（`dcu_scheme_store_storage()` + 4 个 inline 事务助手） |
| AXDR 编解码 | `cmd/dcu/dcu_698_scheme_axdr.c`（`_take_report_unit` / `dcu_698_decode_report_op` / `_encode_report_unit` / `dcu_698_report_encode_axdr`） |
| 分派与对象 | `dcu_698_axdr.c`、`dcu_698_object_model.c`（45 项）、`dcu_698_appconn.c`（44 项）、`dcu_698_class_acq.c`、`dcu_north_adapter.c` |

## 完成证据

- 单元测试 `test_dcu_698_scheme_mgmt` 10/10：Add/Delete/Clear、GET A2（含元素 0 = 最小现有）、重启后持久化、601C/601D 精确字节形状、DAR 分类（编号 0 → 5、通道数越界 → 2、类型 >1 → 7、RECORD → 2、不存在 → 6）。
- 北向证明：对象列表 44 项含 601C/601D（`test_dcu_698_appconn` / `test_driver_dlt698`），北向 ACTION 路由 OI 601C（`dcu_north_adapter.c`），验收用例 `D_00_00_02_000026`（601C 添加 → 601D 回读 → 128 删除）。
- 受影响 ctest 15/15；ASAN `detect_leaks=1`：scheme_mgmt 10、schemes 12、axdr 37 全过、0 泄漏；`run_fanalyzer.py` 3 known / 0 new；`check_build_warnings.py` 0 violations；SRP 守卫干净；`run_guard_selftest.py` 105/105。
- 验收 `./acceptance/run.sh dcu` 2/2 任务合格（dcu_acceptance 3 passed、dcu_northbound 26 passed，退出码 0）。
