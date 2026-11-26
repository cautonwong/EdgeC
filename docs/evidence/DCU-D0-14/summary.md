# DCU-D0-14：消除北向验收“首读电压 0.0”抖动

状态：交 `dcu-verifier` 复核。基线 `feat/dcu d194d894`，分支 `dcu/D0-14-accept-stability`。

## 1. 现象与根因

同一断言在三个合并树上出现过：合并树 d194d894 r2（`test_master_reads_voltage_over_698`，
`assert 231.7 <= 0.5`，实际 0.0）、D3-03b 合并树 r1、D2-07 落地 r1。
产品刚启动、南向首轮抄表尚未完成时，DataHub 影子初值 0.0 表示“还没采到”，不是错误数据；
用例立即断言，等于跟启动时序赛跑。

## 2. 修法（只改 acceptance/dcu 测试）

- 新增共享 helper `_wait_gauge(oad, expected, tol, scale=10.0, deadline_s=15.0)`：
  100ms 间隔重发 GET，第一元素换算后落在 `expected±tol` 即返回；超时用最后值与
  deadline 抛 AssertionError。
- 改造的用例（等待语义，不放宽任何期望/容差）：
  - `test_master_reads_voltage_over_698`：先等 A 相电压进入 231.7±0.5 V，再验数组形状与
    Scaler_Unit。
  - `test_unimplemented_oad_gets_per_item_dar`：在发混合列表帧前先等电压就绪，帧内仍逐项
    断言电压 Data 与缺失 OAD 的 DAR 3。
  - `test_master_reads_current_and_active_power`：电流/功率期望 0，改用 helper（scale=1.0、
    tol=0）表达“等到与台架一致”，不引入固定 sleep。
  - `test_master_uses_spare_di_after_primary_failure` 原先已有带 deadline 的轮询，保持不动。
- 未使用 skip、retry 插件或固定 sleep；未改任何阈值/容差；未改产品代码。

## 3. 验证（三次连续验收，严格按派发命令）

命令：`TESTPLATFORM=/home/edge/test-platform PYTHON=/home/edge/EdgeC/.venv/bin/python3
./acceptance/run.sh dcu`

| 轮次 | 结果 | northbound | 任务门禁 | 残留 DCU |
| --- | --- | --- | --- | --- |
| r1 | exit 0 | 34 passed / 52.75s | 2/2 | 无 |
| r2 | exit 0 | 34 passed / 38.82s | 2/2 | 无 |
| r3 | exit 0 | 34 passed / 42.74s | 2/2 | 无 |

每轮结束 `pgrep -x dcu` 无输出（记录为 `DCU_LEFT=no`）。

分支上另跑：`ctest -R dcu` 37/37；`run_guard_selftest.py` 110/110；SRP 干净；
clean Release build 0 新告警（唯一 warning 为 third-party open62541）。

## 4. 未决项

- helper 默认 deadline 15s：若台架首轮抄表超过 15s，会以最后值失败并给出 deadline，
  不会静默放过；需要更长窗口时由用例显式传 `deadline_s`。
- 电流/功率的期望值是 0，等待只能证明“读到 0”，无法区分“尚未采到”；这是台架 DI 回 0
  的既有语义，本单不放宽也不加强该判据。
