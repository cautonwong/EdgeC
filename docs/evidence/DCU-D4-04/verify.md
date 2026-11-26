# DCU D4-04 验证记录（dcu-verifier，2026-11-26）

分支 `dcu/D4-04-class19`，merge `88e1e550`（tip `d4051496`），基线 `a926964b`。

## 结论

VERIFIED（两轮对抗修复）。Class 19 设备管理（4300、4307–4309）以空掩码对象行 + 只读 ACL 落地，GET 类定义属性 DAR 5、类未定义 DAR 4、未知 OI DAR 6、主站未进 ACL DAR 3；方法 1..6 无处理器回 DAR 5，SET 回 DAR 3，绝不执行复位/初始化。修复轮同时根因解决了 698 从站响应越界：从站独立 10496 B 池（`EDGE_SLAVE_AXDR_MAX + 256`，32 项）、codec 全编码路径 tailroom 守卫、列表项预留 5 B 帧尾、编码失败清长度且引擎只发已封口帧；并修掉 slave_engine 每次请求漏一个 net_buf 引用。

## 对抗评审两轮

1. 首轮：106 项对象列表 4306 B + 帧 = 4328 B 写进 4096 B 池，越界 296 B 写坏下一 net_buf 节点（评审 gdb 探针，我复跑一致：node1 首字节 00 00 00 00 → 02 02 50 f2）。
2. 第二轮（对首轮修复）：列表尾未预留，item 写满后 end 失败留下 10427 B 半帧（L=00 00、无 0x16）仍被引擎发送且请求未消费导致反复重发；要求 item 守卫预留 5 B、失败清长度、引擎 rc<0 不发送。

## 红先绿后（独立复跑）

- 骨架：测试 commit `5688c365` 红 5/45（`116 != 120`、`102 != 106`、行缺失、DAR `3 != 5`）→ tip 绿 45/45。
- 越界轮：`24b49fd5` 红 `out->b.len <= out->b.size is not true`（4328 > 4096）→ 修复后 driver 26/26。
- 帧尾轮：`f1b05ff7` 红 `-1 != 0` 与 `10427 != 0`（26 过 2 败）→ tip driver 28/28、45/45。
- gdb 复验：4096 池 GET 现在 `encode rc=-1 b.len=0` 且下一节点字节不变；生产池列表 item[4] 拒绝回 DAR 后 `end rc=0 b.len=8350 L=9c 20 last=16`；小池同请求 `rc=0 b.len=24 L=16 00 last=16`，无半帧。
- 变异守卫 `DCU-D4-04-list-trailer-reserve` 独立复跑 1/1 sensitive；`DCU-D1-10B-fixed-line-window` 在合并树 1/1。

## 验收

两轮各 3 连绿（2/2 任务，3+35 passed）：首轮修复 3.55/22.80、5.26/22.65、4.47/22.82s；帧尾轮 2.78/22.55、1.90/22.82、1.80/22.73s。原文 `verifier/accept-*.log`。

## 合并树门禁

- 合并树 build 0 第一方 warning、守卫自检全绿、全量 ctest 367/367、变异守卫 `DCU-D4-04-list-trailer-reserve` 与 `DCU-D1-10B-fixed-line-window` 各 1/1 sensitive，原文 `verifier/merged-*.log`。
- ASAN（Debug+ASAN，`detect_leaks=1`）：`test_driver_dlt698`、`test_dcu_698_object_model`、`test_dcu_698_appconn`、`test_dcu_698_axdr` 全绿、无 sanitizer 报告；`run_fanalyzer.py` 3 known / 0 new。原文 `verifier/asan-*.log`、`verifier/fanalyzer.log`。

## HOLD / 缺口

- 4307..4309 的 127..129 记入 resolver 定义但 ACL 每对象方法上限 8（类方法已占 6）不广告，主站 ACTION 回 DAR 3；扩 method_ids 需重算 DRIVER_PRIV_SIZE 预算。
- 无设备描述/版本/复位/初始化 producer 与平台端口。
- `write_async` 引用契约：poll 实现 ref/释放，UDP/HTTP/OPCUA 不持引用；slave 只用 poll io，暂安全，属既有契约缺口。
- 从站池 32 项：IEMS_MAX_VBUS=32 的多从站场景可能耗尽并静默丢响应；DCU 单从站不受影响。
