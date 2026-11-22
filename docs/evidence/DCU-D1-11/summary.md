# DCU-D1-11 北向四类服务验收矩阵

基线与分支：`dcu/D1-11-acceptance`，工作树 `/tmp/dcu-d1-11`，父提交 `1ca7130a`。

## 用例

全部直接发帧、按 APDU 断言；平台点表能表达的才走 `meter.read_all()`。

| ID | 判据 |
| --- | --- |
| 000001 | GET 单点 2000 A2 三相数组 + 2013 Scaler_Unit |
| 000004 | GET 列表 6 个 OAD 一帧逐项回答 |
| 000005 | 2001 电流 / 2004 功率数组 |
| 000002 / 000006 | 6000 / 6012 A2 配置数组未实现 → DAR 5 |
| 000003 | ACTION 8000/129 合法参数 → DAR 2，随后仍能读电压（进程存活） |
| 000009 | ACTION 8000/129 少字段 → DAR 7 |
| 000008 | 列表里混入未列 OAD → 该项 DAR 3，另一项数据保留 |
| 000007 | 5004 日冻结条数（设备模型） |
| 000010 | 5004 GetRecord 双日范围 → 2 行，时标与电量逐行核对 |
| 000011 | SET 3000 A11 enum(1) → DAR 0；GET 读回 enum 1 |
| 000012 | SET 3000 A12 bool(0) → DAR 0；GET 读回 bool 0 |
| 000013 | SET 3000 A11 visible-string → DAR 7 |
| 000014 | SET F200 A4（类定义、实例未声明）→ DAR 5 |
| 000015 | 5004 GetRecord 空范围 → 0 行，帧长 22 |
| 000016 | 同一 socket：预连接读 4400 A2 → CONNECT 结果 0 → 未列 OI DAR 3 → 已列 OI 可读 → RELEASE 成功 → 释放后仍是 DAR 3 |

`_NorthSession` 在一个 unix socket 上连续收发多帧；`_recv_frame` 按长度域收满整帧。CONNECT/RELEASE 状态与会话 ACL 因此可测。

## 暴露的缺陷（已修）

产品对象列表（`cmd/dcu/dcu_698_appconn.c`）停在 D1-18b/D1-18c 之前：3000/3005 没有 A11/A12，F200 没有 A4。结果 SET A11/A12 在 ACL 层被拒（DAR 3），SET F200 A4 也回 DAR 3，而不是对象层的 DAR 5。

修复：

- 事件 ACL 追加 `{11,3},{12,3}`，F200 追加 `{4,3}`；对象计数同步。
- `driver_dlt698.c` 的 `DLT698_ACL_MAX_ATTRS` 8 → 12（事件现在 10 个属性）。
- 保留 3000/3005 其余类定义属性在列表里，主站读未实现位仍由对象层回 DAR 5。

## 门禁

- 干净构建 + `check_build_warnings.py`：0 warning
- `ctest -j`：338/338 通过
- ASAN：7/7 相关目标通过
- `run_guard_selftest.py`：全部通过
- 验收两次：dcu-acceptance 3/3、dcu-northbound 16/16，两个任务退出码均 0
- `git diff --check`：干净
