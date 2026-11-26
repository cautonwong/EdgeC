# DCU-D2-05a：6016/6017 事件采集方案、6018/6019 透明方案、601A/601B 结果（管理平面）

## 范围与拆分

D2-05 拆成管理平面（本单）与执行平面（D2-05b，暂缓）：

- 本单：OI 行与 ACL、GET A2、ACTION 127/128/129/130（6018 另有 131）、SQLite 持久化、DAR 诚实。不实现执行。
- 不做：事件上送（归 D3-02/D3-05b）、透明转发执行（需要 raw I/O 的等待/比对与 ADR-0017 CHOICE[2]）、6013 scheme_type 3 与 poller 执行器。
- 验收：事件上送与透明转发各一例**不在本单**；本单以单元测试 + 北向对象模型证明（对象列表、GET、ACTION 经北向入口）作为出口证据。

## 数据形状

三个方案族共用既有 `dcu_scheme_store`（同一 SQLite 连接、同一事务/回滚风格），不新增第二套存储或调度。6014/6015 继续由 poller 持有 RAM（要执行）；事件/透明/结果没有执行者，**SQLite 是权威**，GET 直接按 OID 查询，不在 RAM 复制一份。

### 事件采集方案（6016/6017）

- 键：方案编号 `unsigned`，1…255；0 保留（OAD 元素 0 = 最小现有编号）。
- 6017 单元：`{编号, 采集方式{类型, 内容}, MS, 上报标识 bool, 存储深度 long-unsigned}`。
- 采集类型只接受标准定义的 0/1/2：0/2 = array ROAD，1 = NULL。ROAD 的 Data 编码 = `0x52 | OAD(4) | 关联个数(1) | 关联 OAD(4×n)`，与该标准 CSD ROAD 示例的成员布局一致。
- MS 只接受 choice 1（全部用户地址）；2…7 保留但不实现，Add 回 DAR 2（能力诚实）。
- MS 字段按标准 H.5.4（印刷页 215–216）编成 `5C <choice>`：它是 ACTION Data 参数 / A2 Data 结构的成员，类型标签 92（0x5C）与 H.5.4 帧里"电表集合数据类型"的标注一致。RSD Selector5 示例（页 207–208）直接写 choice 是协议 SEQUENCE 上下文；产品未实现 Selector5（`DLT698_RECORD_UNSUPPORTED_SELECTOR` → DAR 2），两处不要混用。
- 存储深度原样保存；没有执行者，深度只是配置。
- 方法：127 Add（array）、128 Delete（array 编号）、129 Clear（NULL）、130 UpdateReportFlag（{编号, bool}）。无 131。
- 删除不存在的编号回 DAR 6（与同类的 6014 一致），整批全成全败。

### 透明方案（6018/6019）

- 键：方案编号 `unsigned`，1…255。6018 M128 的编号线上是 long-unsigned，超出 1…255 回 DAR 7（产品编号空间），不静默截断。
- 6019 A2：`{编号, 方案内容集 array, 存储深度}`。
- 方案内容：`{序号 LU, 通信地址 TSA, 开始前脚本 LU, 完成后脚本 LU, 方案控制标志, 方案报文集}`；TSA 编码 = 长度(1) + 字节（2…17），与该标准 MS 示例一致。
- 方案控制标志：`{上报结果并等待后续报文 bool, 等待超时 LU 秒, 结果比对标识 enum, 结果比对参数 structure{特征字节, 截取开始, 截取长度}}`。
- 方案报文：`{报文序号 unsigned, 报文内容 octet-string}`；单帧 ≤ 255 字节、每内容 ≤ 4 帧、每方案 ≤ 4 内容（产品上限，超出回 DAR 2）。
- 方法：127 Add（单个透明方案，可带内容）、128 AddMeterFrame（{编号, TSA, 控制标志, 报文 array}）、129 Delete（{编号, TSA 集合}）、130 Delete（array 编号）、131 Clear（NULL）。
- DAR 诚实：等待后续报文 = true、结果比对标识 ≠ 0 在 Add/AddMeterFrame 回 DAR 2（本单没有执行器，也没有多帧等待/比对能力）。128 对同地址内容做整体替换，保留原序号；新地址追加并分配下一个序号。
- 129 任一地址不存在回 DAR 6 且不做任何删除；130 任一编号不存在回 DAR 6 且不删除。131 只清方案（含内容/报文），保留历史结果。

### 透明结果（601A/601B）

- 结果只能由执行器写入；本单提供存储 API 与 GET 编码，执行器属于 D2-05b。
- 结果：`{存储序号, 方案编号, 执行时间 date_time_s, 通信地址 TSA, 结果集 array {报文序号, 响应时间 date_time_s, 命令结果 octet-string}}`。
- 601A A2 = array<601B 结构>，按存储序号升序。
- 601B A2 的元素索引 = 存储序号（1 起，0 = 最小现有）；不存在回 DAR 6。标准没有规定 601B 的实例寻址，这里按结果数组位置定义并记录。
- 每结果 ≤ 8 帧、每帧 ≤ 255 字节；超出执行器写入时回 DAR 2。
- 601A/601B 不广告方法（A.7 未列方法）；调用未广告方法按类定义/未定义回 DAR 5/4。

## 对象与 ACL

| OI | 类 | 属性 | 方法 |
| --- | --- | --- | --- |
| 6016 | 11 集合 | A2 只读 | 127/128/129/130 |
| 6017 | 8 参数 | A2 只读 | — |
| 6018 | 11 集合 | A2 只读 | 127/128/129/130/131 |
| 6019 | 8 参数 | A2 只读 | — |
| 601A | 11 集合 | A2 只读 | — |
| 601B | 8 参数 | A2 只读 | — |

对象列表新增 6 项，ACL 对象上限从 40 提到 44；驱动会话结构仍小于 1024 字节（静态断言）。

## 持久化

同一 `storage->db` 上新增表（全部 `CREATE TABLE IF NOT EXISTS`，不迁移旧库）：

- `dcu_event_scheme(scheme_id PK, capture_type, report_flag, storage_depth, ms_choice)`
- `dcu_event_road(scheme_id, pos, oad, PK(scheme_id,pos))`
- `dcu_event_road_rel(scheme_id, pos, rel_pos, oad, PK(...))`
- `dcu_transparent_scheme(scheme_id PK, storage_depth)`
- `dcu_transparent_content(scheme_id, pos, seq, address, pre_script, post_script, report_result, wait_timeout, compare_mode, feature_byte, cut_start, cut_len, PK(scheme_id,pos))`
- `dcu_transparent_frame(scheme_id, pos, frame_pos, seq, data BLOB, PK(scheme_id,pos,frame_pos))`
- `dcu_transparent_result(result_id INTEGER PRIMARY KEY, scheme_id, exec_time, address BLOB)`
- `dcu_transparent_result_item(result_id, frame_pos, seq, resp_time, data BLOB, PK(result_id,frame_pos))`

写入走 `BEGIN IMMEDIATE` 事务，失败回滚并回 DAR 2；读取按 OID 查询，顺序确定（编号/结果序号升序）。

## 完成定义（待实现后逐项填证据）

- 静态分析：`check_build_warnings.py` 0 violations。
- 单元测试：`tests/test_dcu_698_scheme_mgmt.c`（事件/透明/结果的 Add、Delete、Clear、UpdateReportFlag、GET、持久化重载、DAR 分类、容量上限）。
- 北向证明：对象列表含 6 个新 OI/类；`dcu_north_accept_action` 与 `dcu_north_resolve_axdr` 全链路通过。
- 既有测试：对象表计数、ACL 计数、catalog 同步更新后全绿。
- 证据：`tests.log`、`runtime.log`（无执行面，记录北向证明输出）、`diffstat.txt`、`comments.md`。

## 完成证据

- 静态分析：`check_build_warnings.py` 0 violations（`tests.log` 末段）。
- 单元测试：`test_dcu_698_scheme_mgmt` 7/7。覆盖事件/透明方案的线格式精确字节、GET 回读、存储重开后仍在、DAR 分类（编号 0 / 采集类型越界 / MS choice 3 / 等待与比对 / 容量）、结果 601A/601B 回读与非公历时间拒绝。
- 受影响回归 15/15：object_model、appconn、catalog、schemes、tasks、axdr、dlt698_appconn、driver_dlt698、report、var、param、value_table、node、inventory、scheme_mgmt。
- ASAN：`ASAN_OPTIONS=detect_leaks=1` 下 scheme_mgmt / schemes / appconn / driver_dlt698 全部 exit 0、0 leak 报告（首轮误用 ctest 的 `detect_leaks=0`；见修复轮）。UBSAN：scheme_mgmt 7/7。
- 北向验收：`acceptance/run.sh dcu` 两个任务全过（acceptance 3 例、northbound 25 例）。新增 `D_00_00_02_000021`（6016 添加→6017 回读→130 改上报标识→128 删除）与 `D_00_00_02_000025`（6018 添加→6019 回读→131 清空），走真实产品 socket。
- ACL 上限：40 → 44（6 个新对象，共 42；驱动会话结构静态断言仍通过）。

## 修复轮

- ASAN 泄漏（`detect_leaks=1`：3968 字节 / 12 次分配）：测试夹具的 `dcu_storage_destroy` 只异步关闭冲刷定时器，原 `_bench_free` 未跑循环就 `uv_loop_close`，句柄未解绑导致 loop 内存泄漏。修复：关闭后 `uv_run(UV_RUN_NOWAIT)` 再 close；`_bench_reopen_storage` 在旧 storage 销毁后同样先跑一圈再 init，避免异步 close 与新 timer 复用同一内存。
- MUST KILL 1：抽出 `_tp_upsert_content`，让"已有地址整体替换、新地址追加"由函数名表达。
- MUST KILL 2：`_encode_result` 直接返回 `EDGE_SLAVE_SRC_*`，删除 `-2` sentinel 与两处调用点映射。
- 仍未改：op struct 按 method 拆 tagged union（形状变更，另立）。

## 未决项

- 事件上送与透明转发执行不在本单，归 D2-05b / D3-02 / D3-05b（plan §12.5 已登记）。
- 601B 的元素寻址标准未规定，本单定义为结果存储序号（1 起，0 = 最小现有）；D2-05b 写结果时沿用。
- 透明方案等待后续报文与结果比对按 DAR 2 拒绝；实现它们需要扩展 raw I/O（超时、多帧、比对），属 D2-05b。
- 结果写入 API `dcu_scheme_store_result_append` 已就绪但生产路径未接；本单只用测试验证读取。

