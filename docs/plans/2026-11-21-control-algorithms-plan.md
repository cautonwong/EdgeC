# 控制与调度算法规划

> 日期：2026-11-21　状态：草案，待评审
> 读者：架构、算法开发、验证
> 关联：`docs/plans/2026-11-21-terminal-common-components-plan.md`、`docs/plans/2026-11-21-protocol-coverage.md`

## How to read this

一个任务行就是一个工作单元。每行都写了负责角色、估时、依赖、交付物和出口证据。出口证据必须是能复现的东西：测试输出、波形、指标记录或提交 SHA。测试通过只是必要条件，完成定义是仓库口径：静态分析、测试、运行时验证证据三者齐全。

## 1. 结论

算法库比预想的完整。PID 与抗积分饱和、陷波与低通、卡尔曼与扩展卡尔曼、LQR、MPC 形式化与 QP 求解、下垂控制、VSG、黑启动、环流抑制、电池 SOC 与 SOH 多方法、削峰、DSP 的 FFT 与窗函数、雷达与电机算法，都已经有代码与部分测试。

缺的不是单个算法，是四件事：

1. 回路装配。抗积分饱和有测试，但模式切换、无扰切换、限幅与输出速率限制没有统一的装配层，各产品自己拼。
2. 约束型调度。削峰与 TOU 有，MPC 形式化有，但缺少通用的约束模型与多目标求解，虚拟电厂与负荷管理要用时会各写一套。
3. 保护判据。过流名字在，方向、零序、选线与重合闸时序没有成体系的实现与验证。
4. 验证证据。单元测试覆盖了 PID 与 MPC 的点，稳定性、边界与实时抖动没有属性测试与预算。

算法规划的原则是：数学留在 `foundation`，装配与策略进 `modules`，产品只做参数与接线。

## 2. 现有算法盘点

| 类别 | 已有实现 | 证据 |
| --- | --- | --- |
| 回路控制 | PID、抗积分饱和、陷波与低通、限幅 | `foundation/include/edge_control.h`、`tests/test_pid_antiwindup.c`、`tests/test_notch_filter.c` |
| 状态估计 | 卡尔曼、扩展卡尔曼、观测器 | `control_kf_*`、`control_ekf_*`、`control_observer_*` |
| 最优控制 | LQR、MPC 形式化、QP 求解 | `control_lqr_solve`、`control_mpc_formulate`、`control_qp_solve_hildreth`、`tests/test_mpc_dispatcher.c` |
| 变流器控制 | 下垂控制 P 与 Q、VSG、黑启动、环流抑制 | `foundation/src/math/control_algo.c`、`modules/edge_power/include/vsg_control.h`、`edge_vsg_engine.h` |
| 电池 | SOC、SOH 六种方法、动态边界、均衡与热管理 | `modules/edge_battery/include/`、`tests/test_battery_*.c` |
| 调度策略 | 削峰、时段套利、MPC 调度器 | `foundation/include/edge_peak_shaving.h`、`tests/test_mpc_dispatcher.c` |
| 信号处理 | FFT、窗函数、RMS、雷达 FMCW、电机 | `foundation/src/math/dsp_algo.c`、`windowing.c`、`radar_algo.c`、`motor_algo.c` |
| 领域算法 | 医疗生理信号、AI 时序故障网络 | `modules/edge_health/include/`、`modules/edge_ai/` |

## 3. 缺口

| 缺口 | 现状 | 影响 |
| --- | --- | --- |
| 回路装配层 | 抗积分饱和与滤波各自独立，模式切换要产品自己写 | 同一套 PID 在三个产品里出现三种切换语义，测试无法复用 |
| 无扰切换与限幅 | 有零散实现，没有统一契约 | 切换瞬间会产生扰动，现场表现为执行器抖动 |
| 约束型调度 | 削峰与 MPC 是专用实现 | 增加需量上限、SOC 窗口、备用容量时，要改多处 |
| 多目标优化 | 无统一权重与优先级模型 | 经济性、安全、寿命三者的取舍散落在配置里 |
| 保护判据 | 过流名字在，方向与零序缺失 | FTU 与 DTU 的保护逻辑要重写，且缺验证 |
| 电能质量聚合 | 有 FFT 与窗函数，无指标聚合 | `edge_pq` 规划中，依赖采样窗口 |
| 验证体系 | 单元测试点状，无属性测试与抖动预算 | 稳定性与实时性靠现场暴露 |
| 实时预算 | 控制回路与其他任务共用 libuv 线程 | 抖动没有量化，无法承诺控制周期 |

## 4. 组件规划

| 组件 | 类型 | 职责 | 依赖 | 复用方 |
| --- | --- | --- | --- | --- |
| `modules/edge_ctrl` | module | 回路装配：PID、抗积分饱和、限幅、速率限制、无扰切换、模式切换 | `foundation` 的 `edge_control.h` | IEMS、配网终端、机器人 |
| `modules/edge_opt` | module | 约束模型、目标函数、QP 与 LP 适配、调度策略注册 | `control_qp_solve_hildreth`、`peak_shaving` | IEMS、负荷管理、虚拟电厂 |
| `modules/edge_fault` | module | 保护判据（过流、方向、零序）与故障记录 | `edge_ctrl` 的限幅与判据接口 | FTU、DTU、故障指示器 |
| `modules/edge_pq` | module | 谐波、闪变、暂降指标 | `edge_capture` 的采样窗口 | 电能质量监测 |
| `modules/edge_battery` | module（已有） | SOC 与 SOH 与均衡 | 无 | IEMS、储能终端 |

算法本身仍放 `foundation/src/math/`，保持纯函数、调用方分配、无全局状态。

## 5. 验证策略

- 黄金向量：每个算法有输入输出对，放在 `tests/` 下的数据文件中，变更时逐位比对。
- 属性测试：稳定性（有界输入有界输出）、不变量（SOC 在窗口内、限幅不越界）、确定性（同输入同输出）。
- 时序预算：控制回路的最坏执行时间与抖动，进 CI 的性能门禁，按产品配置阈值。
- 台架与半实物：FTU 与 DTU 的保护与 FA 在仿真馈线上验证；储能控制用功率硬件在环验证。
- 故障注入：NaN、除零、饱和、传感器跳变，算法必须给出定义过的输出。

## 6. 工作分解

角色：A 架构，AL 算法开发，C C 开发，Q 验证。

### ALG-P0 契约与基线（5 人日，第 1 周）

| ID | 任务 | 角色 | 人日 | 依赖 | 交付物 | 出口证据 |
| --- | --- | --- | --- | --- | --- | --- |
| ALG-P0-01 | 回路装配与调度端口契约 | A | 3 | 无 | 端口头与状态图 | 评审通过 |
| ALG-P0-02 | 现有算法基线测试补齐 | AL+Q | 2 | 无 | 黄金向量清单 | 基线用例全绿 |

### ALG-P1 回路装配（15 人日，第 2 到 4 周）

| ID | 任务 | 角色 | 人日 | 依赖 | 交付物 | 出口证据 |
| --- | --- | --- | --- | --- | --- | --- |
| ALG-P1-01 | `edge_ctrl` 组件与无扰切换 | C | 8 | P0-01 | 组件与 `component.meta` | 切换扰动用例 |
| ALG-P1-02 | IEMS 控制路径迁移 | C | 4 | 01 | 替换产品内回路装配 | IEMS 全量用例绿 |
| ALG-P1-03 | 限幅与速率限制统一 | C | 3 | 01 | 统一实现 | 边界属性测试 |

### ALG-P2 调度与优化（20 人日，第 5 到 8 周）

| ID | 任务 | 角色 | 人日 | 依赖 | 交付物 | 出口证据 |
| --- | --- | --- | --- | --- | --- | --- |
| ALG-P2-01 | 约束模型与目标函数 | AL | 8 | P0-01 | 约束与目标结构 | 约束违反用例 |
| ALG-P2-02 | QP 与 LP 适配 | AL | 6 | 01 | 求解器适配层 | 与解析解对比 |
| ALG-P2-03 | 削峰与 TOU 迁移 | C | 4 | 01 | 策略注册 | IEMS 验收绿 |
| ALG-P2-04 | 多目标优先级 | AL | 2 | 01 | 权重与优先级模型 | 决策记录可复现 |

### ALG-P3 保护判据（20 人日，第 9 到 11 周）

| ID | 任务 | 角色 | 人日 | 依赖 | 交付物 | 出口证据 |
| --- | --- | --- | --- | --- | --- | --- |
| ALG-P3-01 | 过流与方向判据 | AL | 8 | P0-02、C4 采样 | 判据实现 | 波形注入用例 |
| ALG-P3-02 | 零序与选线 | AL | 6 | 01 | 判据实现 | 波形注入用例 |
| ALG-P3-03 | 重合闸时序 | C | 6 | 01、`edge_fa` | 时序实现 | 时序用例 |

### ALG-P4 验证与预算（15 人日，第 12 到 13 周）

| ID | 任务 | 角色 | 人日 | 依赖 | 交付物 | 出口证据 |
| --- | --- | --- | --- | --- | --- | --- |
| ALG-P4-01 | 属性测试与黄金向量 | Q | 6 | P1、P2、P3 | 测试集 | CI 全绿 |
| ALG-P4-02 | 最坏执行时间与抖动预算 | Q | 5 | 01 | 预算表与门禁 | 性能门禁记录 |
| ALG-P4-03 | 半实物验证 | Q | 4 | 02 | 台架报告 | 报告归档 |

合计约 75 人日，13 周。P2 与 P3 可并行。

## 7. 风险与开放问题

1. 控制回路是否允许专用实时线程，还是继续在 libuv 线程上跑。这决定抖动预算的量级。
2. 保护判据的定值来源与切换规则，需与配网终端计划中的定值区组件对齐。
3. 调度优化是否引入第三方求解器，许可证与代码体积要评估。
4. 电池算法与 IEMS 的边界：电池组件已经很大，是否继续按方法拆子组件。
5. 半实物台架的归属与成本。

## Appendix A. 参考

- `foundation/include/edge_control.h`、`foundation/src/math/control_algo.c`
- `foundation/src/math/` 下的 `dsp_algo.c`、`windowing.c`、`radar_algo.c`、`motor_algo.c`
- `modules/edge_power/include/vsg_control.h`、`edge_vsg_engine.h`
- `modules/edge_battery/include/`
- `tests/test_pid_antiwindup.c`、`test_mpc_dispatcher.c`、`test_notch_filter.c`、`test_battery_*.c`
