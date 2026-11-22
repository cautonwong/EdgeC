# DCU-D3-06 Class 8 参数变量

分支 `dcu/D3-06-params`，工作树 `/tmp/dcu-d3-06`，基线 `2f502b50`。

## 白名单与形状

| OI | 属性 | A-XDR | 默认值 | 写入 |
| --- | --- | --- | --- | --- |
| 4000 | 2 | date_time_s | 系统本地时间（读时钟端口） | 时钟端口 `clock_settime` |
| 4000 | 3 | enum 0/1/2/255 | 0 | 落盘 |
| 4000 | 4 | structure 5 × unsigned | 全 0 | 落盘 |
| 4001 | 2 | octet-string ≤ 31 B | 空 | 落盘 |
| 4002 | 2 | octet-string ≤ 31 B | 空 | 落盘 |
| 4003 | 2 | octet-string ≤ 31 B | 空 | 落盘 |
| 4004 | 2 | structure{经度, 纬度, dlu 高度} | 全 0 | 落盘 |
| 401C | 2 | double-long-unsigned | 1 | 落盘 |
| 401D | 2 | double-long-unsigned | 1 | 落盘 |
| 401E | 2 | structure 2 × dlu（金额限值，−2） | 0 | 落盘 |
| 401F | 2 | structure 3 × dlu（透支/囤积/合闸，−2） | 0 | 落盘 |
| 4020 | 2 | structure 2 × dlu（电量限值，−2） | 0 | 落盘 |
| 4021 | 2 | structure 3 × dlu（囤积/透支/合闸，−2） | 0 | 落盘 |

SET 先解到临时值并校验形状与范围，再编码、写库、提交 RAM；任何失败保持旧值。值以规范 A-XDR blob 存入 `dcu_params`（主键 `(oi, attr)`），`dcu_param_audit` 记录 old/new、时间、属性与来源 `dlt698`，两者在同一个 `BEGIN IMMEDIATE` 内。4000 A2 例外：它只调时钟端口，不进 `dcu_params`，成功条件是端口返回 0。

## DAR

| 情况 | GET/SET |
| --- | --- |
| 白名单属性 + 合法值 | 0 |
| 4000 A2 时钟端口返回 0 | 0（GET 随即可读到新时间） |
| 4000 A2 端口缺失或 `clock_settime` 拒绝（EPERM/EINVAL） | 2，时间不变 |
| 属性 1（类定义、实例未实现） | 5（模块）/ 3（线上 ACL 未列） |
| 属性 3/4（非 4000，类上限 2） | 4（模块）/ 3（线上） |
| 类型/结构/字段数/尾随字节不符 | 7，旧值不变 |
| 数值越界（月份 13、枚举 3、经纬度越界） | 7 |
| ACL 无权限 | 3 |
| 表外 OI | 6（模块）/ 3（线上） |
| 落盘失败或无存储端口 | 2，旧值不变 |

## 对象表与 ACL

对象表新增 11 行（`IC_PARAM`、singleton、mask 只含属性 2），总数 30（原 19 行，含 0xF100 ESAM）；Class 20 ACL 新增 11 行，总数 29（4000 列 A2/A3/A4，其余 A2，权限 3）。生成目录把 Class 8 类上限钉在属性 2，因此 4000 的 A3/A4 不在对象 mask 里，由参数模块在通用 mask 检查之前接管。

驱动 ACL 从每对象内联数组改为 16 字节紧凑对象（属性 id 1..16 各 2 bit 权限 + 8 个方法 id），上限 30 对象/16 属性/8 方法，30 × 16 = 480 B。ACL 29 个对象超过旧的 28 对象上限，这正是抬高上限的原因；属性 id 0 或大于 16 直接拒绝（fail-closed）。新增 `_Static_assert(sizeof(dlt698_session_t) <= 1024)` 守住 `DRIVER_PRIV_SIZE`。

## 保持关闭的位

- 4000 方法 127（广播校时）：驱动的 ACTION 统一走 `dcu_north_adapter.c` 的 `accept_action`，该文件由 D1-20a 持有。方法位不声明，callee 返回 DAR 5/4。
- 4005 及以后未列入白名单的 Class 8 OI：没有对象行，线上 ACL 先回 DAR 3。

## 4000 的时间来源

4000 A2 的 GET 调 `dcu_clock_port_t::now`（系统实现读 `CLOCK_REALTIME`），SET 调 `set`（系统实现 `mktime` + `clock_settime(CLOCK_REALTIME)`）。端口由 `dcu_node.c` 在组合根注入，测试注入可控假端口。SET 返回 0 只在系统时钟真的接受新值之后；没有端口、`mktime` 失败或 `clock_settime` 返回 EPERM/EINVAL 都回 DAR 2，GET 仍读到旧时间。不存在只改内存、下一次 GET 就"成功"的路径。

4000 A3/A4 与 4001–4004、401C–4021 只有存储与审计语义：SET 成功 = 已持久化并可回读，产品尚无功能消费这些值，设备行为不变。这一点写进设计文档；接入功能时另立任务并补行为测试。

## 验证

- 干净构建（`ninja -C build clean` 后全量重建）+ `check_build_warnings.py`：0 warning
- 全量 `ctest -j8`：341/341
- ASAN（`-DENABLE_ASAN=ON` 全量构建）：5 个相关目标全过
- `run_guard_selftest.py`：全部通过
- 验收：`dcu-acceptance` 3/3、`dcu-northbound` 20/20（新增 000022），两个任务退出 0，`ACCEPTANCE_TASKS` 2/2
- `run_fanalyzer.py --paths cmd/dcu,adapters/io_drivers/src`：0 新报告
- `git diff --check`：干净

测试：`tests/test_dcu_698_param.c`（13 行逐行往返、4000 A2 时钟端口成功/拒绝、A3/A4 范围、octet 0/31/32、geo 嵌套、限值顺序、DAR 5/4/6/7、保存失败、重启重载、审计 old/new + attr、4000 A2 不进 `dcu_params`），以及对象表/ACL/驱动/axdr 的回归用例；验收 `D_00_00_02_000022` 绑定 DAR 0 与时间实际改变。
