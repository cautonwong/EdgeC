# PCS 厂商能力边界：feilton / xiankong

状态：审计记录（2026-11-26，WBS P3-02c）。基于 `feat/iems` f7420c8e 的 EdgeC 与 v3 源码逐操作对照。产品/现场对寄存器语义的确认未做，单独列在 §5，不阻塞本切片。

## 1. 范围与证据

- v3 参考实现（只读）：`/home/edge/iems_v3/src/adapter/pcs/feilton_pcs.c`、`xiankong_pcs.c`。
- EdgeC 适配器：`cmd/iems/vendor/iems_vendor_pcs_a.c` 的 `feilton_ops`（177-256）、`cmd/iems/vendor/iems_vendor_pcs_b.c` 的 `xiankong_ops`（192-278）。
- 框架：`cmd/iems/vendor/iems_vendor_semantics.c` 的 `read_tag`/`write_tag`/`read_int`/`write_int`/`read_int_as_double` 与扩展派发；公共接口 `cmd/iems/vendor/iems_vendor_semantics.h` 未改。
- 行为钉在 `tests/test_iems_vendor_semantics.c` 的 `test_feilton_vendor`、`test_xiankong_vendor`。
- v3 的 `dev_get_dev_tag_int` 读失败返回 0（`src/cc_lc_com/collector-api.c:459-463,474-479`），而 EdgeC 的 `read_int` 读失败返回 -1（`iems_vendor_semantics.c:18-25`）。这个差异是所有“缺失 tag”行为的来源，属有意更严格，见 §4。
- v3 枚举 `PE_ADP_OFF=0`、`PE_ADP_ON=1`、`PE_ADP_STANDBY=2`（`src/adapter/adapter_define.h:10-12`），与 EdgeC `IEMS_VENDOR_OFF/ON/STANDBY` 数值一致。

## 2. feilton 能力表

v3 证据 `feilton_pcs.c`，EdgeC 证据 `iems_vendor_pcs_a.c`。

| 操作 | v3 行为 | EdgeC 行为 | 结论 |
| --- | --- | --- | --- |
| `init` | 返回 0（5-8） | `vendor_init` 返回 0 | 一致 |
| `set_power` | 读 `SetPwrMode`（int）。0 写 `WrtActPowSet=power`；512 把 `power/3` 分别写 `WrtActPowSetA/B/C`；其他值 -1（10-29） | 同三寄存器、同 1/3 切分；额外把读失败/写失败变成 -1（181-200） | 寄存器与值一致，失败路径更严格 |
| `set_repower` | -1（31-34） | `unsupported_set_repower` -1，不碰 I/O | 一致 |
| `set_mode` | -1（36-39） | `unsupported_set_int` -1，不碰 I/O | 一致 |
| `set_onoff` | OFF 写 `Stop=5461072`；ON 且 `StartReady==1` 写 `Start=5395790`；ON 但未就绪 -2；其他 onoff -1（41-66） | OFF/ON 同寄存器同值；`StartReady` 读失败 -1，可读但不等于 1 返回 -2（204-219） | 已修正：原先未就绪返回 -1，现与 v3 一致 |
| `set_connect` | -1（67-70） | `unsupported_set_int` -1 | 一致 |
| `get_power` | -1（72-75） | `unsupported_get_double` -1 | 一致 |
| `get_repower` | -1（77-80） | `unsupported_get_double` -1 | 一致 |
| `get_mode` | -1（82-86，`return 0` 是死代码） | `unsupported_get_int` -1 | 一致 |
| `get_onoff` | 读 `StopState`，0 → ON，1 → OFF，其他 -1（88-107） | 同映射；读失败 -1（222-236） | 可读值一致，读失败更严格 |
| `get_connect` | -1（109-112） | `unsupported_get_int` -1 | 一致 |
| `set_value` | 五方法按名派发，其他交给模板扩展 `adapter_function_set`（114-141） | 五方法按名派发，未知函数 -1（`dispatch_set_value`） | 有意不移植模板扩展 |
| `get_int_value` | `get_onoff`/`get_mode`/`get_connect` 全部误派到 `feilton_pcs_get_onoff`（143-162） | `get_onoff` 走同名方法；`get_mode`/`get_connect` 未实现返回 -1 且不读 tag | 有意不复制 v3 误派 |
| `get_double_value` | `get_power`/`get_repower` 均 -1，其他交给模板扩展（164-179） | 两者 -1，未知函数 -1 | 有效行为一致 |

feilton 支持的寄存器：`SetPwrMode`（读）、`WrtActPowSet`、`WrtActPowSetA/B/C`（写 double）、`StartReady`（读）、`Stop`=5461072、`Start`=5395790、`StopState`（读）。不支持的 vtable 方法共 7 个，全部返回 -1 且不触 I/O：`set_repower`、`set_mode`、`set_connect`、`get_power`、`get_repower`、`get_mode`、`get_connect`。

## 3. xiankong 能力表

v3 证据 `xiankong_pcs.c`，EdgeC 证据 `iems_vendor_pcs_b.c`。

| 操作 | v3 行为 | EdgeC 行为 | 结论 |
| --- | --- | --- | --- |
| `init` | 返回 0（7-10） | `vendor_init` 返回 0 | 一致 |
| `set_power` | `dev_set_dev_tag_int("ActPowSet", power)`，double 隐式截断为 int，恒返回 0（12-16） | `write_int("ActPowSet", (int)power)`，显式截断；返回写结果（197-199） | 截断语义一致，写失败更严格 |
| `set_repower` | -1（18-21） | `unsupported_set_power` -1 | 一致 |
| `set_mode` | -1（23-26） | `unsupported_set_int` -1 | 一致 |
| `set_onoff` | OFF 写 `OnOffSet=0`；ON 且 `StartReady` 非 0 写 `OnOffSet=1`；ON 但 `StartReady` 为 0 返回 -2；其他 onoff -1（28-53） | 同寄存器同值；读失败 -1，可读值非 0 视为就绪（201-216） | 可读值一致，读失败更严格 |
| `set_connect` | OFF 写 `WorkMode=0`，ON 写 `WorkMode=1`，其他 -1（54-70） | 同寄存器同值（218-226） | 一致 |
| `get_power` | -1（72-75） | `unsupported_get_double` -1 | 一致 |
| `get_repower` | -1（77-80） | `unsupported_get_double` -1 | 一致 |
| `get_mode` | -1（82-85） | `unsupported_get_int` -1 | 一致 |
| `get_onoff` | 读 `ModeOnOffStatus`，1 → ON，0 → OFF，其他 -1（87-106） | 同映射；读失败 -1（228-242） | 可读值一致，读失败更严格 |
| `get_connect` | 读 `CurWorkMode`，1 → `*connect=0`（断开），2 → 1（并网），其他 -1（108-126） | 同映射；读失败 -1（244-258） | 一致 |
| `set_value` | 五方法按名派发，其他交给模板扩展（128-155） | 五方法按名派发，未知函数 -1 | 有意不移植模板扩展 |
| `get_int_value` | `get_onoff`/`get_mode`/`get_connect` 全部误派到 `xiankong_pcs_get_onoff`（157-176） | `get_onoff` 同名；`get_mode` 未实现 -1；`get_connect` 读 `CurWorkMode` | 有意不复制 v3 误派 |
| `get_double_value` | 两个功率读接口 -1，其他交给模板扩展（178-193） | 两者 -1，未知函数 -1 | 有效行为一致 |

xiankong 支持的寄存器：`ActPowSet`（写 int）、`StartReady`（读）、`OnOffSet`（写 0/1）、`WorkMode`（写 0/1）、`ModeOnOffStatus`（读 0/1）、`CurWorkMode`（读 1/2）。不支持的 vtable 方法共 5 个，全部 -1 且不触 I/O：`set_repower`、`set_mode`、`get_power`、`get_repower`、`get_mode`。

### 3.1 set/get WorkMode 不对称

v3 原样如此，EdgeC 保留，测试钉住：

- 写侧 `set_connect` 写 `WorkMode`。并网（connect=1）写 1，断开（connect=0）写 0（`xiankong_pcs.c:54-70`）。
- 读侧 `get_connect` 读 `CurWorkMode`。1 映射为断开（0），2 映射为并网（1）（`xiankong_pcs.c:108-126`）。
- 因此同一个数值 1 在写侧表示并网，在读侧表示断开；`CurWorkMode` 没有 0 值映射。`test_xiankong_vendor` 显式钉住“写入 1 后按同值读回得到断开”这一不对称，防止后续顺手把两侧改成对称。

## 4. 有意保留的偏差

1. **读失败即 -1，不复制 v3 的缺省 0。** v3 `dev_get_dev_tag_int` 对任何读失败都返回 0，于是缺失 tag 被当作合法值：feilton `set_power` 缺失 `SetPwrMode` 会按统一给定写 `WrtActPowSet`；feilton `set_onoff`/xiankong `set_onoff` 缺失 `StartReady` 会走 -2；feilton `get_onoff` 缺失 `StopState` 会报 ON；xiankong `get_onoff` 缺失 `ModeOnOffStatus` 会报 OFF。EdgeC 一律返回 -1 且不写，通信故障不再伪装成设备状态。例外是 xiankong `get_connect` 缺失 `CurWorkMode`，v3 的缺省 0 本来也落到 -1 分支，外部结果相同。
2. **写失败向上传播。** v3 的适配器忽略 `dev_set_dev_tag_*` 的返回值，只在取值非法时返回 -1；EdgeC 返回 -1（feilton 的分相写在第一个失败处停止）。测试用 `fake.fail_write = -1` 钉住。
3. **扩展接口按同名方法派发。** v3 两家把 `get_mode`/`get_connect` 误派到 `get_onoff`；EdgeC 的 `dispatch_get_int_value` 按函数名调用同名 vtable 方法，所以 feilton 的 `get_mode`/`get_connect` 经扩展接口返回 -1，xiankong 的 `get_mode` 返回 -1、`get_connect` 读 `CurWorkMode`。这是框架级决定，对所有厂商一致，不在本切片改动。
4. **不移植 v3 模板扩展。** v3 的 `set_value`/`get_int_value`/`get_double_value` 在五方法之外回落到 `adapter_function_set`/`adapter_function_get_*`；EdgeC 没有模板扩展层，未知函数名返回 -1。若现场依赖模板扩展，需要另行设计，不在本切片。

## 5. UNCHECKED（产品/现场）

以下全部来自源码推定，未经产品文档、设备手册或现场报文确认。

feilton：

1. `SetPwrMode` 是否只有 0（统一给定）和 512（分相给定）两个合法值，以及寄存器是否所有型号都存在。
2. `StartReady` 的取值语义，v3 只认 1；其他值（含 2）在线设备是否表示就绪。
3. 启动/停机魔数 `Start=5395790`、`Stop=5461072` 的字节序、模型适用范围。
4. `StopState` 除 0/1 外是否还有合法状态。
5. `WrtActPowSet*` 的功率单位、符号与缩放，分相是否真的按 3 等分（不平衡现场可能不是）。
6. 缺失 `SetPwrMode` 时应当阻塞（EdgeC 现状）还是按统一给定继续（v3 行为）。
7. 现场是否使用五方法之外的模板扩展函数。

xiankong：

1. `StartReady` 的“非 0 即就绪”假设是否成立。
2. `OnOffSet` 的 0/1 极性、是否需要启动确认或复位。
3. §3.1 的 `WorkMode`/`CurWorkMode` 不对称是否为真实寄存器语义，还是 v3 的笔误；写侧是否应改用其他 tag。
4. `CurWorkMode` 除 1/2 外是否有合法状态（0、3 等）。
5. `ActPowSet` 的整数缩放、符号约定，以及小数功率截断是否符合现场预期。
6. v3 没有实现功率/无功读回，设备是否存在可读 tag。
7. 缺失 `StartReady` 时应按 v3 缺省值返回 -2，还是按 EdgeC 现状返回 -1（当前选择 -1）。

## 6. 测试钉住

`tests/test_iems_vendor_semantics.c`：

- `test_feilton_vendor` 钉住 `SetPwrMode` 0/512/非法值/缺失、三个功率写寄存器与 1/3 值、`Stop`/`Start` 魔数、`StartReady` 0/2 → -2、缺失 → -1、`StopState` 0/1/其他/缺失、7 个 unsupported 方法不触 I/O、扩展接口派发。
- `test_xiankong_vendor` 钉住 `ActPowSet` 正负小数截断、`OnOffSet` 0/1、`StartReady` 0 → -2、2 → 就绪、缺失 → -1、`WorkMode` 写 0/1、`ModeOnOffStatus` 与 `CurWorkMode` 映射及缺失、set/get WorkMode 不对称、5 个 unsupported 方法不触 I/O、扩展接口派发。

## 7. 残余风险

- 测试使用 fake tag 端口，只证明适配器语义，不证明现场设备接受这些寄存器名和值。
- I/O 端口的错误码除 0/-1 外原样透传（例如 `write_int` 直接返回端口返回值）；测试只钉住 -1。
- double 到 int 的截断只覆盖可表示区间；越界值是 C 的未定义行为，未定义保护。
- v3 的缺失 tag 行为是 `dev_get_dev_tag_int` 的默认值副作用，不是设计意图；现场行为可能与本表不同。
- 本切片未运行全量 ctest、架构 gates 或 ASAN，验证限于 `test_iems_vendor_semantics`。
