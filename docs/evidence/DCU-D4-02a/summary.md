# DCU-D4-02a：Class 13 只读属性与诚实 DAR（切片 1）

状态：交 `dcu-verifier` 复核。基线 `feat/dcu bdd87c89`，分支 `dcu/D4-02a-class13`。

## 1. 标准依据（A.9，docs/698_parsed.md 8716 起；PDF 为准）

- 8000 远程控制（IC 13）：A2 structure{拉闸电流门限 double-long-unsigned 换算 -4, 超电流保护延时
  long-unsigned 分钟}；A4 告警状态 bit-string(8)；A5 命令状态 bit-string(8)。方法 127/128 NULL，
  129/130/131 为跳闸/合闸/明文合闸（本切片不执行）。
- 8001 保电（目录 IC 8）：A2 enum{解除,保电,自动保电}；A3/A4 long-unsigned 分钟；A5 自动保电时段。
- 8100 终端保安定值、8101 功控时段、8102 功控告警时间、810E 控制对象、810F 跳闸轮次、
  8110 电控定值（目录 IC 8）；8103 时段功控（IC 13，A2 方案集 + 方法 127）。
- 810E/810F/8110 在 A.9 只有对象名，没有属性/方法定义。

## 2. 诚实来源评估（OI/属性 → 来源）

| OI | 属性 | 标准内容 | 产品来源 | 结论 |
| --- | --- | --- | --- | --- |
| 8000 | A2 | 拉闸电流门限/保护延时 | `dcu_peripheral_telecontrol` 有 `do_states`/`io_ops` 结构，但产品没有任何实例、没有 `set_do` 注入，`dcu_telecontrol_init` 只在自身文件出现 | DAR 2 |
| 8000 | A4/A5 | 告警/命令状态 bit-string | 无继电器输出通道，状态从未由真实 DI/DO 更新 | DAR 2 |
| 8001 | A2-A5 | 保电状态/时长/时段 | 无保电硬件或配置存储 | DAR 2 |
| 8100 | A2 | 保安定值 | 无功率控制引擎实例（`dcu_load_control` 同样未接线） | DAR 2 |
| 8101/8102 | A2 | 功控时段/告警时间 | 无来源 | DAR 2 |
| 8103 | A2 | 时段功控方案集 | 无来源 | DAR 2 |
| 810E/810F/8110 | A.9 未定义属性 | 仅对象存在性 | 无定义、无来源 | A2 回 DAR 5（类定义但未实现） |

没有用 0 值/空数组冒充真实状态：仓库既有 6032/6033 先例是 DAR 2，本切片沿用。

## 3. 对象/ACL 决策（不动上限）

- 8000 已在 ACL（仅方法）；本切片把 A2/A4/A5 加进它已有的 ACL 行，**对象计数不变**，ACL 仍 60/60。
- 8001/8100/8101/8102/8103/810E/810F/8110 只进对象模型，不进 ACL/Class 20：驱动会话
  `DRIVER_PRIV_SIZE` 1024、ACL 对象 16 字节、cap 60，新增 8 项必然超限；按 D3-02 裁决
  不改 foundation 头、不放宽上限。代价：主站访问这些 OI 回 DAR 3（ACL 拒绝），已在验收
  `D_00_00_02_000034` 断言 8001 A2 → DAR 3。
- 对象模型 66 → 74（新增 8 行）；ACL 60 不变。
- 8000 方法 127–131 保持既有诚实拒绝（无继电器 OAD→表地址映射，DAR 2）；8001/8103 的
  方法类定义但未广告 → DAR 5（未实现），本切片不执行控制。

## 4. 实现

- `dcu_router.h`：新增 8001/8100/8101/8102/8103/810E/810F/8110 OI 宏。
- `dcu_698_object_model.c`：8000 加 A2/A4/A5 掩码；新增 8 个对象行（8001/8103 IC 13，
  其余目录 IC 8；810E/F/8110 无掩码）。
- `dcu_698_appconn.c`：8000 ACL 行加 {2,1},{4,1},{5,1}，计数不变。
- `dcu_698_axdr.c`：`dcu_north_resolve_axdr` 为上述 OI 增加显式分支：无来源属性回
  `EDGE_SLAVE_SRC_TEMPORARY`（DAR 2），类定义未暴露属性回 CLASS_MISMATCH（DAR 5），
  未定义回 UNDEFINED（DAR 4），未知 OI 仍 NOT_FOUND（DAR 6）。
- 属性 2 的 SET 不在本切片：本切片是“只读属性 + 诚实 DAR”；ACL 对 8000 A2 只给只读权，
  SET 会被 ACL 拒绝（DAR 3），没有假装可写。

## 5. 测试

- 单测 `test_class13_readonly_dar`（`tests/test_dcu_698_axdr.c`）：逐 OAD 断言
  8000 A2/A4/A5、8001 A2/A5、8100/8101/8102/8103 A2 → DAR 2；8000 A3 → DAR 5；
  8000 A6 → DAR 4；810E A2 → DAR 5；0x9999 → DAR 6。
- 对象计数 `test_dcu_698_object_model` 66→74；8000 期望掩码 {2,4,5}；目录一致性测试保持绿。
- 验收 `D_00_00_02_000034`：8000 A2/A4/A5 GET → DAR 2；8001 A2 → DAR 3（ACL 未收录）。

## 6. 门禁（最终树）

- clean Release warning 0；`ctest -R dcu` 37/37；ASAN `detect_leaks=1` 3/3 零泄漏；
  fanalyzer 3 known / 0 new；SRP 干净；guard 110/110。
- 验收 `acceptance/run.sh dcu` 2/2，3 + 34 passed（33 旧 + 新 000034）。首次重跑曾撞
  `test_task_monitor_matches_bench_failures` 既有 6035 时窗抖动，干净重跑 34/34。

## 7. 未决项

- 控制执行（129/130/131 真下发）与属性 2 SET 明确不在本切片。
- 8001/8100-8103 进 ACL 需要产品拍板替换低价值项或批准提高 `DRIVER_PRIV_SIZE`；当前代价是
  主站 DAR 3。
- 810E/810F/8110 的标准定义在 A.9 只有名字，属性/方法需产品点位表补全。
- 继电器/功控真实来源（`dcu_peripheral_telecontrol`、`dcu_load_control` 实例化与 IO 注入）
  另立任务；接上真实来源后这些 DAR 2 才能改为真实值。
