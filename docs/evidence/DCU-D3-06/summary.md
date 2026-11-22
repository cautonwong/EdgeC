# DCU-D3-06 Class 8 参数变量

分支 `dcu/D3-06-params`，工作树 `/tmp/dcu-d3-06`，基线 `4233a7c0`。

## 白名单与形状

| OI | 属性 | A-XDR | 默认值 | 写入 |
| --- | --- | --- | --- | --- |
| 4000 | 2 | date_time_s | 本次上电的系统本地时间 | RAM 影子，不落盘 |
| 4000 | 3 | enum 0/1/2/255 | 0 | RAM 影子 |
| 4000 | 4 | structure 5 × unsigned | 全 0 | RAM 影子 |
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

SET 先解到临时值并校验形状与范围，再编码、写库、提交 RAM；任何失败保持旧值。值以规范 A-XDR blob 存入 `dcu_params`，`dcu_param_audit` 记录 old/new、时间与来源 `dlt698`，两者在同一个 `BEGIN IMMEDIATE` 内。

## DAR

| 情况 | GET/SET |
| --- | --- |
| 白名单属性 + 合法值 | 0 |
| 属性 1（类定义、实例未实现） | 5（模块）/ 3（线上 ACL 未列） |
| 属性 3/4（非 4000，类上限 2） | 4（模块）/ 3（线上） |
| 类型/结构/字段数/尾随字节不符 | 7，旧值不变 |
| 数值越界（月份 13、枚举 3、经纬度越界） | 7 |
| ACL 无权限 | 3 |
| 表外 OI | 6（模块）/ 3（线上） |
| 落盘失败或无存储端口 | 2，RAM 不变 |

## 对象表与 ACL

对象表新增 11 行（`IC_PARAM`、singleton、mask 只含属性 2），总数 29（原 18 行）；Class 20 ACL 新增 11 行，总数 28（4000 列 A2/A3/A4，其余 A2，权限 3）。生成目录把 Class 8 类上限钉在属性 2，因此 4000 的 A3/A4 不在对象 mask 里，由参数模块在通用 mask 检查之前接管。

驱动 ACL 从每对象内联数组改为 16 字节紧凑对象（属性 1..16 各 2 bit 权限 + 8 个方法 id），上限 28 对象/16 属性/8 方法，28 × 16 = 448 B；新增 `_Static_assert(sizeof(dlt698_session_t) <= 1024)`。

## 保持关闭的位

- 4000 方法 127（广播校时）：驱动的 ACTION 统一走 `dcu_north_adapter.c` 的 `accept_action`，该文件由 D1-20a 持有。方法位不声明，callee 返回 DAR 5/4。
- 4005 及以后未列入白名单的 Class 8 OI：没有对象行，线上 ACL 先回 DAR 3。

## 4000 的时间来源

`dcu_698_param_init` 用 `time()` + `localtime_r` 生成一次上电时间；SET 只改 RAM 影子，不回写系统时钟，也不落盘。重启后 GET 反映新的系统时间，而不是上一次 SET 的旧值。若产品后续接入时钟服务，这里改为读取该服务并保留同样的 A-XDR 形状。

## 验证

- 干净构建 + `check_build_warnings.py`：0 warning
- 全量 `ctest -j`：340/340
- ASAN：8/8 相关目标
- `run_guard_selftest.py`：全部通过
- 验收两次：`dcu-acceptance` 3/3、`dcu-northbound` 19/19，两个任务退出 0，`ACCEPTANCE_TASKS` 2/2
- `run_fanalyzer.py --paths cmd/dcu,adapters/io_drivers/src`：0 新报告（子代理运行）
- `git diff --check`：干净

测试：`tests/test_dcu_698_param.c`（13 行逐行往返、4000 A2/A3/A4 范围、octet 0/31/32、geo 嵌套、限值顺序、DAR 5/4/6/7、保存失败、重启重载、审计 old/new、4000 不落盘），以及对象表/ACL/驱动/axdr 的回归用例。
