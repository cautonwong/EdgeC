# DCU-D1-20a 北向 ACTION 路由

基线 `2b7bf199`（D2-03 已合入，且已含 D3-04a 的两条时间口径决定），分支 `dcu/D1-20a-action-route`。
原分支基于 `42516ae6`，先后 rebase 到 `3d971fbb` 与 `2b7bf199`，与 D2-02/D2-03 的改动两边都保留。

目标：`dcu_north_accept_action` 不再对非 8000 的 OI 一律回 `NOT_FOUND`（DAR 6），
把"方法是否存在 / 参数是否可接受"交回对象表与 handler；适配器不再当第二个 DAR 源。

## 最终路由顺序（`dcu_north_accept_action`）

```c
if (oi == OI_METER_ARCHIVE)   return dcu_698_accept_archive_action(omd, axdr, axdr_len); // 6000, D2-02
if (oi == OI_TASK_SCHEDULER)  return dcu_698_accept_action_axdr(omd, axdr, axdr_len);    // 6012, D2-03
if (oi != OI_REMOTE_CONTROL)  return dcu_698_call(omd, axdr, NULL);                     // 其它 OI, D1-20a
... dcu_698_decode_action_param(...) ... return dcu_698_call(omd, &param, NULL);         // 8000, A.9
```

| OI | 入口 | 参数形态 |
| --- | --- | --- |
| 6000 | `dcu_698_accept_archive_action`（D2-02） | 解 A.7 档案操作 |
| 6012 | `dcu_698_accept_action_axdr`（D2-03） | 解 127/128/129/130 为 `poller->task_op`；方法 1 只收 NULL |
| 8000 | 适配器 A.9 解码 + `dcu_698_call` | 已解码的 `dcu_action_param_t` |
| 其它 | `dcu_698_call(omd, axdr, NULL)` | 原始 A-XDR，由 handler 解读或拒绝 |

- 方法存在性由对象表 `method_mask` 决定：未知 OI -> DAR 6，类/实例定义未暴露 -> DAR 5，
  类里没有 -> DAR 4；参数可接受性由 handler 决定。适配器不再复制类表。
- 6012 的 D2-03 解码器原本要求 Data 精确结束；而端口按既有约定交付
  "Data + 可选一个时间标签字节"（8000/6000 的解码器也容忍这一个字节）。
  本单让 6012 的 `dcu_698_decode_task_op` 与方法 1 的 NULL 检查同样接受一个尾随
  时间标签字节，否则真实驱动帧会被误判 DAR 7。
- 尾随字节只接受 `0x00`（表 121 的"没有时间标签"）。有值的 TimeTag 要求服务器
  做传输时效判定（6.1.1.5），本实现不做，按 DAR 7 拒绝并在 8000/6000/6012 三处
  共用 `_time_tag_absent()`；不静默忽略时标。验收用例覆盖有值 TimeTag -> DAR 7。
- 因此 D2-03 的 `test_wrong_tag_and_count_return_7` 里"尾随一个 0 字节"的用例改成
  "时间标签 + 多余尾随字节"（两个字节）：一个时间标签是合法线格式，超出的才是垃圾。
  该用例的意图（尾随垃圾 -> DAR 7）不变，未被删除或放宽。

## 测试（`test_dcu_698_axdr`）

- `test_action_non8000_class_defined_unexposed_dar5`：5004 方法 3，DAR 5。
- `test_action_unknown_oi_dar6`：OI 0x9999 方法 1，DAR 6。
- `test_action_non8000_reaches_handler`：6012 方法 1（复位）-> DAR 0。
- `test_action_6012_method127_valid_body_dar0`：6012 方法 127，A2 单元体（与
  `tests/test_dcu_698_tasks.c` 同形状）经真实驱动帧 -> DAR 0；并断言持久化端口
  收到的 op 是解码后的 `method=127 / task_id=7 / TI 60`，证明到 handler 的不是原始字节。
- `test_action_6012_method127_truncated_body_dar7_no_state_change`：6012 方法 127
  截断体 -> DAR 7；断言持久化端口未被调用、`poller` 原有任务逐字节不变、
  任务 7 未出现。该用例同时是 ASAN 回归（旧路径把原始 A-XDR 当 `dcu_task_op_t` 读会越界）。
- 8000 的 127/128/129/130/131 与"未知方法 126 / 多余字节"用例不变，仍通过。
- 路由用例用一份放行 5004 m3 / 6012 m1、m127 / 9999 m1 的测试 ACL，越过驱动的
  ACL 前置门禁，单独验证适配器与对象表的 DAR（生产 ACL 仍会先拒未授权方法）。

## 验收用例

`acceptance/dcu/tests/test_dcu_northbound.py::D_00_00_02_000019`
（"6012 ACTION 127 改周期 / 127 重下发 / 128 删除，A2 全程可回读"）：
标签由 `待接入` 改为 `北向`，删除 DAR 6 时 `pytest.skip` 的分支。现真实跑通
（加 -> 读回 60 秒 -> 改为 120 秒 -> 删除 -> 读回不存在）。

## 组合根预算

`cmd/dcu/dcu_north_adapter.c` 由 203 行增到 204 行（6012 分支一行），已在
`ci/baseline/composition_root_budget.txt` 登记 `203 -> 204`（只增账本，不缩）。

## 门禁

见 `tests.log`。
