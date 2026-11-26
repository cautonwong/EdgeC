# DCU-D2-07 范围评估：601E/601F 采集规则表

状态：评估稿，未写实现。基线 `feat/dcu 7ef4b2f8`，分支 `dcu/D2-07-rules`。

## 1. 依据与依赖

- 计划 §5.4：规则行 `(oad, protocol, protocol_id, scale, length)`，DLT645 有主用与备用 DI 两行；一个 OAD 在三协议下的量纲只在这张表定义。
- 计划 §8 D2-07：替换三套硬编码点表，支持主用与备用 DI；出口证据=同一 OAD 在 DLT645 与 DLT698 的量纲对拍 + 备用 DI 回退用例；实现文件 `dcu_698_rules.{h,c}`。
- 计划 §5.8：`dcu_driver_adapter.c` 只保留协议元数据登记，点表搬去规则表。
- 标准（`docs/698_parsed.md` 8551/8559，原文表 A.7）：601E 类 11，属性 2=array 采集规则，方法 127 Add(array 采集规则)、128 Delete(array CSD)、129 Clear(NULL)；601F 类 8，属性 2=structure { CSD, 规则描述 }，规则描述=structure 3 { AcqCmd_2007{主用 DI array octet-string(SIZE(4)), 备用 DI array octet-string(SIZE(4))}, AcqCmd_1997{主用 DI array octet-string(SIZE(2)), 备用 DI array octet-string(SIZE(2))}, AcqCmd_Trans{Frame octet-string} }；主用 DI 抄不到时用备用 DI。
- CSD（表 32）：CHOICE [0] OAD / [1] ROAD。现有 6014/6015 编成 `5B 00 OAD`；601F 沿用同一 CSD 编码。
- D2-01（COM 迁移）未合并；2026-11-22 operator 已拍板 P2–P5 先在 `cmd/dcu` 内实现，故不触发 §9.5 #1。

## 2. 数据形状取舍

### 2.1 存储行（内部，计划口径）

`dcu_rule(oad, protocol, role, protocol_id, scale, length, frame)`，主键 `(oad, protocol, role)`：

- `protocol`：1=DLT645-2007，2=DLT645-1997，3=DLT698，4=ModbusRTU，5=Trans。
- `role`：0=主用/单行，1=备用。645 的主用/备用各一行；698/Modbus 只有 role 0；Trans 的行为 role 0。
- `protocol_id`：645=DI（uint32，0x02010100 形式）；1997=2 字节 DI；698=OAD；Modbus=寄存器地址（长度走 `length`，不再用 `MODBUS_NATIVE` 打包值入库）。
- `scale`、`length`：产品量纲与宽度；645 的备用行继承同 OAD 主用行的 scale/length（标准把备用 DI 定义为同量替换）。
- `frame`：仅 protocol=5 使用，BLOB；产品没有透明执行路径，只做管理面存取（与 D2-05a 透明方案同一口径）。

### 2.2 线缆形状与投影

- 601F A2 = `5B 00 OAD` + 规则描述 structure 3。
  - 2007 主用/备用 DI：`protocol_id` 4 字节按 DI0 低字节先（与 `driver_dlt645.c` 的 `_put_di` 一致；0x02010100 → `00 01 01 02`），编成 `array octet-string` 的 0 或 1 个元素。
  - 1997 主用/备用：2 字节低字节先。
  - Trans：frame BLOB。
- 601E A2 = array，按 oad 升序分组投影；601F A2 的元素索引 = 601E 数组位置（0=第一项）。**这是产品约定，不是标准明文**（规则没有编号可寻址），列未决项 1。
- 每个 DI 数组只支持 0/1 个元素（产品一个 OAD 一个协议号）；127 收到 >1 → DAR 2。
- ROAD CSD（记录型列）无采集执行路径 → 127 DAR 2；GET 不合成 ROAD。

### 2.3 scale/length 的线缆缺口（关键取舍）

601F 线缆只有 DI/frame，没有 scale/length；计划行却包含它们。取舍：

- 种子行（来自现三张表）携带产品已知的 scale/length，迁移后行为逐值不变（含 645 电流 0.001、698 电流 0.01、Modbus 功率 0.001 等差异）。
- 127 只接受产品已有元数据的 CSD（即种子覆盖的 OAD）；主用 DI 可在该 OAD 上改写，scale/length 沿用该 OAD 的原值；备用行继承主用。未知 CSD → DAR 2（不能标定就不假装能抄）。
- 698/Modbus 行没有 601F 线缆槽位，只能由种子产生；主站无法通过 601F 配置它们，这与标准一致（标准只定义 645-2007/1997/Trans 命令）。

## 3. 存储与运行时唯一来源

- 复用 `dcu_scheme_store` 的同一 SQLite 连接与 schema 初始化（`dcu_scheme_store_init`），新增 `dcu_rule` 表；事务风格沿用 `dcu_scheme_store_internal.h`。按 SRP 拆出 `cmd/dcu/dcu_rule_store.c`（apply/query），store 主文件只留 schema 与绑定，避免再顶 1500 行。
- 播种：新库首次 init 时从 `dcu_698_rules.c` 的种子数组写入（种子内容=现三张表的逐行拷贝）；用独立 meta 键记录“规则已播种”，主站 Clear 后重启不复活（与方案播种的 `seeded` 同机制）。
- 运行时唯一来源：新模块 `cmd/dcu/dcu_698_rules.{h,c}` 持有调用方分配的固定容量快照 `dcu_rule_cache_t`（嵌入 `dcu_node`），init 时从库加载，127/128/129 成功提交后重建；`dcu_driver_adapter_oad_to_proto/proto_to_oad/scale` 全部改读该快照，三张静态表与三段查找循环删除，只留协议元数据登记。快照发布走模块内 mutex/原子指针，热路径不碰 SQLite（现有调用点在 `dcu_bus_source._submit/_on_done`、`dcu_node` 落库、`dcu_698_axdr` 编码，均在 `dcu_node` 的 uv loop 线程；快照加锁为跨线程兜底）。
- 不新增调度、不新增第二套驱动查找；`edge_api`/驱动仍只认协议号（ADR-002）。

## 4. 对象行、ACL 与 cap

| OI | 类 | 属性 | 方法 |
| --- | --- | --- | --- |
| 601E | 11 集合 | A2 只读 | 127/128/129 |
| 601F | 8 参数 | A2 只读 | — |

- 对象模型 45 → 47；ACL 44 → 46。
- `EDGE_SLAVE_ACL_MAX_OBJECTS` 44u → 46u（`foundation/include/edge_slave_plane.h`，驱动 ACL 数组共用此宏）。该头**不在** `scripts/check_core_readonly.sh` 的受保护清单（清单只列 `modules/edge_core/` 与 `contract_data_types.h` 等 8 个头），无需白名单登记；需在证据里记录这一核对。
- D2-08 的六个状态 OI 再各自提升，不在本单预支。

## 5. 三张表的迁移路径

1. 把 `s_dlt698_points`（16 行）、`s_dlt645_points`（15 行）、`s_modbus_points`（15 行）逐行搬到 `dcu_698_rules.c` 的种子数组（字段更名到规则行；Modbus 的 `MODBUS_NATIVE(addr,len)` 拆成 protocol_id=addr、length=len）。
2. `dcu_scheme_store_init` 建表后播种；`dcu_698_rules_init(storage, cache)` 载入快照并 `attach`，由 `dcu_node` 组合根在驱动启动前调用。
3. `dcu_driver_adapter.c` 删除三张表与查找循环，函数体变成对 `dcu_698_rules` 的查询调用。
4. 对拍测试锁定迁移前后每个 (协议, OAD) 的 protocol_id/scale/length 逐值相等；现有 `test_dcu_driver_adapter.c` 的断言保持通过（fixture 改为先建库/快照）。

## 6. 备用 DI 回退

- 唯一执行缝是 `cmd/dcu/dcu_bus_source.c`：`_submit` 取主用协议号；`_on_done` 失败且协议为 DLT645 且规则存在备用行时，以备用协议号重发一次，仍失败才回调错误。重试复用现有 bus ctx/refcount，不排队、不新调度。
- 正反用例：主用失败→备用成功（回调成功，值按备用行 scale 标定）；主用失败→备用也失败（回调错误，只发两次，不循环）；非 645 协议不重试。
- 现有 `dcu_bus_source` 没有单测替身（`edge_api_read_tag` 是全局）。实施时把“是否/用什么协议号重试”抽成纯函数（读快照）直接测；端到端再用验收模拟从站补一条。

## 7. 影响面

| 文件 | 变更 |
| --- | --- |
| `cmd/dcu/dcu_698_rules.{h,c}` | 新增：种子、快照、attach/init/refresh、查找 API |
| `cmd/dcu/dcu_rule_store.c` | 新增：`dcu_rule` schema 读写、事务 apply、查询 |
| `cmd/dcu/dcu_scheme_store.{h,c}` | 规则类型、schema 串联、store 绑定 |
| `cmd/dcu/dcu_driver_adapter.c` | 删三张表与查找循环，委托规则模块 |
| `cmd/dcu/dcu_698_scheme_axdr.c` | 601E/601F 编解码（现 654 行，预计 +~180） |
| `cmd/dcu/dcu_698_axdr.c` | GET/ACTION 分派 |
| `cmd/dcu/dcu_698_object_model.c` / `dcu_698_appconn.c` | +2 对象 / +2 ACL |
| `cmd/dcu/dcu_698_class_acq.c` / `dcu_north_adapter.c` | 601E 处理器与北向路由 |
| `cmd/dcu/dcu_node.c` | 规则 init/attach 组合根接线 |
| `cmd/dcu/dcu_bus_source.c` | 645 备用 DI 一次重试 |
| `foundation/include/edge_slave_plane.h` | cap 44→46 |
| `tests/` | 新 `test_dcu_698_rules.c`；扩展 scheme_mgmt / driver_adapter / object/appconn/driver 计数；验收 601E/601F 一例 |

不改 `adapters/io_drivers`、`modules/edge_core`、ADR、公共组件；不新增调度。

## 8. §9.5 核对

1. 依赖未合并：D2-01 未合并，但 operator 已定 P2–P5 先在 `cmd/dcu` 实现 → 不阻断。
2. 标准依据：601E/601F 形状见 §1，齐全。
3. 受保护头：`edge_slave_plane.h` 不在门禁清单 → 不阻断。
4. 不放宽测试/门禁。
5. 不改 ADR/公共组件/跨产品契约。
6. 证据可复现（本机 CI 等价门禁）。

结论：可以实施，范围限于 `cmd/dcu` + 一处非受保护的 foundation 宏。

## 9. 未决项（请 verifier 定）

1. **601F A2 元素索引语义**：标准没有规则编号。提议 elem = 601E 数组位置（0=第一项，按 oad 升序）；替代方案：601F 不可读（只读 601E），但 catalog 已列 601F A2。
2. **1997/Trans 规则**：管理面接受并存储（标准合法，D2-05a 先例），执行面不实现；还是 127 直接 DAR 2 拒绝？提议存储 + 证据注明“仅管理面”。
3. **备用 DI 的 scale/length**：提议继承主用行；若 verifier 要求每行独立标定，则种子必须为备用行提供产品已知值（当前三张表没有备用 DI 数据）。
4. **127 的 DI 合法集**：提议只接受产品已有元数据的 CSD（未知 CSD DAR 2），主用 DI 可改写；若要求接受任意 DI，需要一份 645 DI 数据格式表作为新的产品知识来源（超出本单）。
5. **回退重试的测试层级**：纯函数单测 + 验收端到端是否足够，还是必须给 `edge_api_read_tag` 做可注入替身。

## 10. 实施顺序与估算

1. 失败测试 commit：规则行查找/对拍、601E/601F 字节向量、备用回退纯函数、对象/ACL 计数。
2. 实现 commit：store/规则模块/迁移/编解码/对象接线/回退。
3. 证据 commit：SRP、guard 105、受影响 ctest、ASAN detect_leaks=1、验收 2/2、warning 0、fanalyzer 3 known/0 new。

估算：2 到 3 个实现会话；主要不确定项是未决 1/4/5。

## 11. 切片 1 落地：行为不变的迁移

verifier 决定（2026-11-23）：1) 601F elem=601E 数组位置、越界 DAR 6、同一投影函数；2) 1997/Trans 仅管理面；3) 备用行继承主用 scale/length；4) 127 只接受种子覆盖的 OAD；5) 回退=纯函数单测+验收端到端。

本切片落地：

- 新增 `cmd/dcu/dcu_698_rules.{h,c}`：迁移前三张点表的逐行种子（46 行）、调用方分配的快照 `dcu_rule_cache_t`、`find_by_oad/find_by_proto`；`dcu_698_rules_detach()` 由节点销毁时解绑，之后回落到内置种子。
- 新增 `cmd/dcu/dcu_rule_store.{h,c}`：`dcu_rule(oad, protocol, role, protocol_id, scale, length)` + `dcu_rule_meta` 建表与“只播种一次”、快照载入；复用 `dcu_scheme_store_internal.h` 的事务助手与同一 `dcu_storage_t`，不并入 `dcu_scheme_store_init`（方案 store 不认识规则表）。
- `cmd/dcu/dcu_driver_adapter.c`：三张静态表与三段查找循环删除，函数改为协议字符串→枚举 + Modbus (地址,寄存器数) 打包，数值全部来自规则模块。
- `cmd/dcu/dcu_node.c`：节点结构内嵌 `dcu_rule_cache_t`，在方案/任务装载之后调用 `dcu_698_rules_init`；销毁时 detach。
- 测试：新增 `tests/test_dcu_698_rules.c` 5 条（未 init 种子查找、46 行逐值对拍、跨协议物理量等价、适配层读规则源、重开库单次播种）；`tests/CMakeLists.txt` 注册。

验证（本切片）：SRP 干净；guard 105/105；受影响 ctest 17/17（含新 rules 5/5）；ASAN `detect_leaks=1`：rules 5 / schemes 12 / scheme_mgmt 10 / node 3，0 泄漏；验收 2/2（3 + 26）；`check_build_warnings` 0；`run_fanalyzer` 3 known / 0 new。

## 12. 切片 2 落地：601E/601F 对象与编解码

- 601E 类 11：A2 = array 采集规则；127 Add(array)、128 Delete(array CSD)、129 Clear(NULL)。601F 类 8：A2 只读。线缆按表 A.7：CSD `5B 00 OAD`；2007 主/备 DI = array octet-string(4)（DI0 低字节先）；1997 主/备 = array octet-string(2)；Trans = Frame octet-string。
- 投影唯一来源：`dcu_rule_store_projected_count/oad` 以标准行（协议 1/2/5）JOIN 该 tag 的 698 行（协议 3、主用），按 `protocol_id`（= 主站可见 CSD OAD）升序；601E 数组与 601F elem 用同一投影。`dcu_rule_unit_t.csd` 携带外部 CSD，编码写 CSD；`unit_load` 仍按内部 tag 取行。elem 越界回 DAR 6。698/Modbus 内部行不在线缆上现形。相别行内部 tag（如 0x20000300）与 CSD（0x20000201）不同，CSD 只从 698 行取。
- 127 语义：解码出的 CSD 先经 698 行反查到内部 tag（映射不到，包括影子 tag、未实现 OAD，DAR 2），再做种子覆盖检查；ROAD CSD DAR 2；重复 protocol_id（其他 OAD 已占用或同批重复）DAR 3；三块全空 DAR 7；2007 块有内容但主用 DI 为空 DAR 7；DI 数组 >1 元素 DAR 2；1997/Trans 接受并存储，执行面不实现（仅管理面）。scale/length 沿用该 OAD 的 645 主用行（种子或已配置），备用/1997 继承主用。
- 128 同样先把 CSD 映射到内部 tag（映射不到 DAR 2；映射到但无标准行 DAR 6）；128/129 只删协议 1/2/5 的行，698/Modbus 内部行保留。127/128/129 提交后载入临时快照、成功才替换；失败不改快照。
- 对象 45→47、ACL 44→46、`EDGE_SLAVE_ACL_MAX_OBJECTS` 44u→46u（`foundation/include/edge_slave_plane.h`，不在 `check_core_readonly.sh` 受保护清单）。
- 编解码在 `dcu_698_scheme_axdr.c`（现 821 行），分派在 `dcu_698_axdr.c`，601E 处理器在 `dcu_698_class_acq.c`，store apply/query 在 `dcu_rule_store.c`（469 行）；`dcu_scheme_store.c` 保持 1398 行。
- `dcu_rule` 增加 `frame BLOB` 列；切片 1 的库通过 `PRAGMA table_info` 检查 + `ALTER TABLE` 补列，保留现场规则。
- 评审修复（verifier 复现）：原实现把内部影子 tag 当 CSD，标准主站发 0x20000201 会被拒、发 0x20000300 反被接受。现按上述 698 行映射修正；投影按外部 CSD 排序，601E 编码与 601F elem 都输出外部 CSD。

实现中发现并修复：投影计数最初用 `COUNT(*)` 而非 `COUNT(DISTINCT oad)`，配了备用 DI 后 601E 数组长度虚增 1，编码第 16 项时 unit_load 失败回 DAR 2；单元测试锁定“添加备用后 601E 仍是 15 项、长度 488”的回归。

验证（评审修复后最终树）：SRP 干净；guard 105/105；受影响 ctest 17/17（scheme_mgmt 15、rules 9）；ASAN `detect_leaks=1`：scheme_mgmt 15 / rules 9 / schemes 12 / node 3，0 泄漏；验收 2/2（dcu_acceptance 3、dcu_northbound 28：`D_00_00_02_000027` 总加 OAD 127→601F→128；`D_00_00_02_000028` 相别 CSD=0x20000201 127→601F→128、影子 tag 回 DAR 2、601E 不含 0x20000300）；`check_build_warnings` 0（clean Ninja Release）；`run_fanalyzer` 3 known / 0 new。

复现（verifier 场景的等价物，`tests.log` 末段原文）：投影 `[3] internal=0x20000300 csd=0x20000201`、`[4] csd=0x20000202`；`count=15 has_csd(0x20000201)=1 has_shadow(0x20000300)=0`；`127 csd=0x20000201 rc=0; 127 csd=0x20000300 rc=-24`。

## 13. 切片 3 落地：645 备用 DI 一次重试

- 唯一执行缝 `cmd/dcu/dcu_bus_source.c`：`_submit` 仍按主用 DI 提交；`_on_done` 失败时，若协议为 DLT645、本请求未重试过、且规则快照有该 tag 的 2007 备用行，则用备用 protocol_id 重发一次。bus ctx 增加 `retried` 标志，复用同一 ctx 与回调；重试期间不回调，重试提交失败则归还 ctx 并回调一次错误；第二次失败后不再重试。
- 纯函数 `dcu_698_rules_retry_with_spare(protocol, oad, already_retried, &spare_id)` 只读规则快照、不碰 SQLite；单元覆盖：无备用不重试、有备用重试、非 645 不重试、已重试不再重试、未知 tag 不重试，以及“主用失败→备用失败”总提交次数 = 2。注释审查后只保留声明处合同。
- 量纲：成功值按 `dcu_driver_adapter_scale(m->protocol, c->proto_id)` 换算，`c->proto_id` 在重试时已换成备用 DI；测试断言备用行 scale/length 与主用行相等（127 继承）。
- 线程/重入核对：`on_finish` 在引擎 I/O 回调里触发，此时引擎已把 `current_cmd` 置空、`arbiter_on_io_done` 尚未执行，`arb->lock` 未持有；从回调内 `edge_api_read_tag` 只是把新命令入队，随后由 `arbiter_on_io_done` 正常派发。未改 `edge_api` 或公共组件。

端到端（已用台架 DI 失败验证，窗口边界已修）：窗口起点取“控制生效后走完一个轮询周期”的静止点，随后再收一个完整周期作为窗口；每块表只归一化两端各一个半周期（前导备用=主用在快照前，尾部主用=备用在快照后），中间必须严格“主用→备用”成对，因此备用后出现第三次会被抓住。全局 per_di 差值在块边界可正可负（run2 main>spare、run3 spare>main），不作次数判据。连续 3 次 `./acceptance/run.sh dcu` 全绿（各 3+29），日志与 `spare_window.json`/`bus.json` 原文见 `tests.log`。`acceptance/dcu/dcu_product.py` 增加 opt-in 控制文件 `runtime/<product>/<task>/sim645_control.json`（`fail` 列表回 645 异常帧，`payloads` 按 DI 给原始值；默认无文件 = 原行为），并把请求序列记入 `bus.json` 的 `reads`（含表地址与 DI）。新用例 `D_00_00_02_000029`：601E 127 给电压 OAD 配置主用 0x02010100 + 备用 0x02010105；台架让主用回异常帧、备用回 220.0（BCD 低字节先 `0022`）；等待采集后北向读 0x20000200 得到 2200（220.0×0.1），并按每块表断言请求序列严格“主用→备用”成对、主用与备用计数一对一（无第三次），结束后恢复规则与台架。验收 2/2，northbound 29 条通过（含新用例）。

线程核对与 TSAN：DCU 产品只在 `uv_default_loop()` 上跑（`cmd/dcu/main.c`），698 从站与总线回调都挂这个 loop；`cmd/dcu` 无 `uv_thread_create`/`pthread_create`，规则快照的读写都在同一 loop 线程。TSAN 在本机不可运行：`vm.mmap_rnd_bits=32` 且 `/proc/sys` 只读、`setarch -R` 被拒（TSAN 报 unexpected memory mapping）。因此 TSAN 是证据缺口；已用单线程接线核对与 ASAN/UBSAN 之外的门禁替代，未额外加锁（保持 verifier 的“无锁快照”决定）。

验证（本切片最终树）：SRP 干净；guard 105/105；受影响 ctest 17/17（rules 10、scheme_mgmt 15）；ASAN `detect_leaks=1`：rules 10 / scheme_mgmt 15 / schemes 12 / node 3，0 泄漏；验收 2/2（dcu_acceptance 3、northbound 29，新用例 `D_00_00_02_000029` 验证主用失败→备用成功且只重试一次；现有用例确认正常 645 采集不受影响）；warning 0（clean Ninja Release）；fanalyzer 3 known / 0 new。TSAN：本机不可运行（见上），证据缺口已记录。
