# DCU F5S 验证记录（dcu-verifier，2026-11-26）

分支 `dcu/F5-snapshot-identity`，merge `5b446fcc`（tip `4848b34a`），基线 `b1907349`。

## 结论

VERIFIED。事件 A6 关联数据快照按事件自己的稳定 slot 身份取表：`_capture_record_snapshots` 的发生与恢复两处都传 `ev->device_id`，编码走新 `dcu_698_var_encode_axdr_device(oad, device_id, ...)`；OAD 低字节只作属性/元素选择。北向 GET 的旧编码器语义不变。原型单一来源在 `dcu_698_var.h`（返工轮移入，两处本地声明删除）。

## 红先绿后（独立复跑）

- 红：测试 commit `d99574b6` 在旧码上 `1000 != 1500`（test_dcu_advanced_modules.c:568），2 例中 1 红，原文 `verifier/red-test.log`。
- 绿：返工 tip `4848b34a` 同用例 2/2，`ctest -R 'dcu|dlt698'` 45/45（116.44s），原文 `verifier/green-test.log`、`verifier/green-ctest.log`。

## 验收（worktree `build`，Release）

3 连绿，2/2 任务：3 passed + 35 passed；时长 10.43/16.28、8.10/15.83、9.63/20.03s。原文 `verifier/accept-{1,2,3}.log`。

## 对抗评审（事件生命周期/身份）

NO FINDINGS。评审用 gdb 在两个测试里观察编码调用：`device=0` 两次（发生+恢复）、`device=3` 两次；旧编码器在北向 resolver 之外零命中；read_shadow/datahub 全链 uint32，无截断；原型仅 var.h 一处声明、var.c 一处定义。原文要点记入 `verifier/review.md`。

## 合并树门禁

- 合并树 build 0 第一方 warning，守卫自检全绿；全量 ctest 367/367，原文 `verifier/merged-build.log`、`verifier/warnings.log`、`verifier/merged-guards.log`、`verifier/merged-ctest.log`。
- ASAN 目标集：`test_dcu_advanced_modules` 2/2、`test_dcu_698_axdr` 47/47，无 sanitizer 报告，原文 `verifier/asan-*.log`。
- `run_fanalyzer.py` 3 known / 0 new（与 D4-10 合并树门禁一起跑，原文 `verifier/fanalyzer.log`）。

## 残留

- 评审指出 `dcu_event_monitor.c:231/247` 的 `evt->last_start_time` 是每个事件 OI 一份全局值，恢复记录的发生时间可能来自另一只表的最近一次发生；这是本单未触碰的既有身份缺口，已记 §12.5。
