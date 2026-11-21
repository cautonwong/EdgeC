# ADR 0013: 698 从站 A-XDR 端口——结构属性的取值与写入通道

## 状态

Proposed（2026-11-21，待架构评审）。

## 上下文

### 问题

从站面的取数端口只传标量。`foundation/include/edge_slave_plane.h:36-50` 的
`edge_point_value_t` 只有 `value`、`raw`、`width`、`type`、`quality`、`ts_ms`，
`resolve`（`:66`）只能返回一个数。698 的 Class 7、9、10、20 属性是数组或结构，
Class 8 的参数也是结构；用标量端口只有两个结果：截断，或者由驱动猜。

同一处还有第二个缺口。`adapters/io_drivers/src/driver_dlt698.c` 的从站交换：

- `:223` 写明 `(void)session; /* 无跨帧状态 */`；
- 只处理 `DLT698_SVC_GET_REQ`（`:257`）与 `DLT698_SVC_ACTION_REQ`（`:315`），
  最后 `:360` 直接返回 -1，没有 `DLT698_SVC_SET_REQ`（`dlt698_codec.h:32`）分支；
- 没有 GetRecord、RSD、Report 与 Class 20 会话。

编码侧同样按标量固定：`_encode_axdr_value`（`:187`）只认 3 种类型标签，写入
调用方给的长度 8 字节缓冲；`GetRequestNormal` 用 `uint8_t data[8]`（`:307`），
`GetRequestNormalList` 用 `uint8_t bufs[DLT698_GET_LIST_MAX][8]`（`:270`）。

标准本身没有模糊点：7.3 节给每类属性，附录 A 给实例结构，附录 H 给字节样例。
缺的是端口形状与驱动分支。

### 约束

- 驱动不认识点位模型（ADR-0010、ADR-0011）；类型标签与结构语义归产品。
- 组件不用动态内存，缓冲由调用方提供。
- IEC104 与 Modbus 的标量路径不得改变。
- `edge_slave_source_ops_t` 是 `foundation/include/` 的公开消费端口，新增字段必须
  先证明向后兼容。

对应计划 B1 与 D1-12（`docs/plans/2026-11-21-dcu-698-production-plan.md` 11.3、12.2）。

## 决策

### 1. 给端口追加两个可选回调

```c
int (*resolve_axdr)(void *ctx, uint32_t oad, uint8_t *buf, size_t cap, size_t *len);
int (*accept_axdr)(void *ctx, uint32_t oad, const uint8_t *axdr, size_t len);
```

- `resolve_axdr`：产品把该 OAD 的**完整 A-XDR 值**写进 `buf`，**包含类型标签**；
  `*len` 返回实际长度。`cap` 由调用方给。
- `accept_axdr`：产品收到 OAD 对应的**原始 A-XDR 值**（含标签），自己解码与校验；
  驱动不解释结构。
- 两个字段都追加在 `edge_slave_source_ops_t` 末尾，现有字段顺序不动。

### 2. 回落只认 NULL，不认返回值

- 698 GET：`src->resolve_axdr != NULL` 时只走 A-XDR；为 NULL 才回落到 `resolve`。
- 698 SET：`src->accept_axdr != NULL` 时只走 A-XDR；为 NULL 才回落到 `accept(double)`。
- `resolve_axdr` / `accept_axdr` 非 NULL 但返回非 0 时，是**真实失败**（无此对象、
  类型不支持、参数非法），驱动回 DAR 或不应答，**不得再回落标量**。
  否则一个已实现结构属性的产品会因为一次解码错误悄悄退回截断值，主站看到的是错值。
- ACTION 不变：已有 `accept_action`（`edge_slave_plane.h:104`）本来就是原始 A-XDR。

### 3. 缓冲上限与记录分页

- `EDGE_SLAVE_AXDR_MAX` 首版 **2048** 字节，是**单条属性**的上限，由调用方提供缓冲。
- 超过 2048 的记录型属性（Class 9 冻结、Class 7/24 事件等）走 GetRecord 分页，
  不把整张表塞进一个 Get 响应。分页由 D1-15 实现。
- 不允许 `N × 2048` 的列表缓冲。`GetRequestNormalList` 最多 8 项
  （`DLT698_GET_LIST_MAX`，`dlt698_codec.h:46`），驱动必须逐项编码进响应，
  或把单项预算压到响应剩余空间；栈占用受 `check_stack_usage.py` 约束。
- 不引入 `malloc`；缓冲可来自栈、组合根预留的请求 scratch，或 `net_buf` 内部空间。

### 4. 驱动的职责边界

- 驱动只做：解 OAD、调端口、把返回的 A-XDR 原样封进响应、把 SET 的 A-XDR
  原样交给端口。
- 驱动不做：类型标签推断、结构拆分、点位语义、单位换算。
- IEC104 与 Modbus 驱动不读新字段，标量路径零变化。

## 兼容性分析（硬问题）

`grep -rn "edge_slave_source_ops_t" --include=*.c --include=*.h` 的全部直接初始化点：

| 位置 | 初始化方式 | 结论 |
| --- | --- | --- |
| `tests/test_modbus_write_guard.c:369` `k_accept_ok` | designated `.accept` | 安全 |
| `tests/test_modbus_write_guard.c:370` `k_accept_reject` | designated `.accept` | 安全 |
| `tests/test_dlt645_codec.c:296` `src` | designated `.resolve` / `.accept` | 安全 |
| `tests/test_dlt645_codec.c:330` `src` | designated | 安全 |
| `tests/test_dlt645_codec.c:360` `src` | designated | 安全 |
| `tests/test_protocol_loopback.c:117` `s_src` | designated | 安全 |
| `tests/test_driver_iec104.c:205` `k_slave_src` | designated `.resolve` / `.enumerate` | 安全 |
| `tests/test_driver_iec104.c:560` `k_slave_src_with_close` | designated `.resolve` / `.enumerate` / `.close_link` | 安全 |
| `tests/test_iems_full_stack.c:198` `s_iec_src` | designated | 安全 |
| `tests/test_iems_full_stack.c:205` `s_mb_src` | designated | 安全 |
| `tests/test_cia402.c:78` `s_slave` | designated `.resolve` / `.accept` | 安全 |
| `cmd/dcu/dcu_north_adapter.c:120` `src` | designated `.resolve` / `.accept_action` | 安全 |
| `cmd/iems/iems_north_adapter.c:138` `ops` | designated `.resolve` / `.accept` | 安全 |
| `cmd/iems/iems_north_adapter.c:240` `ops` | designated `.resolve` / `.accept` / `.enumerate` / `.deadband` | 安全 |

非初始化、只做拷贝或转传的位置：

| 位置 | 行为 | 结论 |
| --- | --- | --- |
| `cmd/iems/iems_north_iec104.c:116` | `edge_slave_source_ops_t ops = *iems_north_iec104_source_ops();` 后补 `ops.close_link` | 整结构拷贝，新字段随之复制，安全 |
| `modules/edge_session/src/session_manager.c:26` `slave_src` | 运行期 `*ops` 赋值（`:59`）或 `memset(0)` | 安全 |
| `modules/edge_io/src/engines/slave_engine.c:150` | 只接收并转传指针 | 安全 |
| `modules/edge_io/src/bus_orchestrator.c:122` | 只接收并转传指针 | 安全 |
| `tests/test_iems_iec104_north.c:177` | 取现有 ops 指针 | 安全 |
| `include/bus_engine.h:33` | 仅函数指针签名 | 安全 |

**结论：不存在位置初始化（positional initializer）。** 所有直接初始化都是 designated
initializer；C 标准要求未指定的成员零初始化，所以把两个函数指针追加在**末尾**
是源码兼容的：现有 14 个初始化点无需改动，新字段自动为 NULL，走标量回落。

这条安全性依赖“只追加、不插入、不重排”。一旦把字段插到中间，designated 初始化
仍能编译，但任何位置初始化都会把后续字段错位。因此本 ADR 同时定一条规则：
`edge_slave_source_ops_t` 只允许末尾追加；需要重排时按破坏性变更处理，一次改完
所有初始化点并在 PR 说明。

二进制 ABI：本仓库的产品、驱动、测试一起编译，无跨版本共享对象；结构体只在进程内
按指针传递，不放进共享内存或持久化格式。`sizeof` 变化不影响现有调用方。若未来有
外部插件按旧布局编译，必须提供版本号或能力位；本 ADR 不覆盖该场景。

## 被否方案

1. **只保留标量端口。** Class 7、9、10、20 的结构属性无法表达，只能截断或猜。
   截断时主站收到“完整但错误”的值，比明确失败更危险。
2. **把对象模型放进 698 驱动。** 驱动要认识 OI、类、属性和点位语义，违反
   ADR-0010 与 ADR-0011 的依赖方向；同一套对象模型还要在 645、104 重写一遍。
3. **把对象模型放进 foundation。** foundation 只应回答“字节怎么编解码”，
   放进领域对象会把协议层与领域层粘死，其他复用方被迫携带 DCU 语义。
4. **把 2048 字节缓冲内联进 `edge_point_value_t`。** 每个标量点位、每次 IEC104
   调用都背上 2 KiB，栈和拷贝成本全面膨胀；标量路径被结构需求污染。
5. **驱动内动态分配，按需扩容。** 违反组件禁用动态内存的规则，且把内存失败
   引入实时应答路径。

## 影响与后续

- **D1-13**：`foundation/include/edge_slave_plane.h` 追加字段；
  `cmd/dcu/dcu_north_adapter.c` 实现 `resolve_axdr` / `accept_axdr`，接对象表。
  出口用例：标量回落、结构属性、无动态内存。
- **D1-14**：`driver_dlt698.c` 补 `SET_REQ`，解 A-XDR 后调 `accept_axdr`，按标准回 DAR。
- **D1-15**：编解码与驱动补 GetRecord、RSD 与分页；超 2048 的记录走这条路径。
- **D1-16**：补 REPORT.notification 的端口回调与产品上报源。
- **D1-17**：从站交换补跨帧会话状态与 Class 20，支撑应用连接与 ACL。
- 本 ADR 不实现代码。D1-13 落地时必须重跑
  `python3 ci/scripts/run_guard_selftest.py --root .` 与
  `ctest -R 'dcu|dlt698'`，并补一条“追加字段后所有现有初始化点仍为零初始化”的测试。

## 参考

- `foundation/include/edge_slave_plane.h`
- `adapters/io_drivers/src/driver_dlt698.c`
- `foundation/include/dlt698_codec.h`
- `docs/design/DCU_698_OBJECT_MODEL.md` 4.2、4.5
- `docs/design/slave_plane_architecture.md`
- `docs/adr/0010-eliminate-global-contracts-and-tagid.md`
- `docs/adr/0011-northbound-two-family-model.md`
- `docs/plans/2026-11-21-dcu-698-production-plan.md` 11.3 B1、D1-12 到 D1-17
