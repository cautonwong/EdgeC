# DCU-D2-03 Class 10 任务配置（6012/6013）

基线与分支：`dcu/D2-03-tasks`，已 rebase 到 `d21cb6f9`（含 D3-05a）；实现分支 `dcu/D2-03-impl` 合并入库。

rebase 冲突：`cmd/dcu/dcu_698_axdr.c` 的 GET 分派。两边都保留：事件类（D3-05a）走 `dcu_698_event_encode_axdr`，6012/6013 走 `dcu_698_task_encode_axdr`；对象表与 ACL 两边行并存，ACL 对象数 18（事件属性 11 项 + 6012 五方法 + 6013 A2）。

## 目的

6012 只广告方法 1（复位）和 127（单方案），参数靠结构体强转、只改 RAM；6013 根本不存在。本单按 A.7 把任务配置单元做成可增删改查、可持久化、周期变更立即生效，且任何拒绝都有诚实 DAR。

## 方法 → 效果

| 位 | 广告 | 行为 |
| --- | --- | --- |
| 6012 M1 复位 | 是 | 运行时复位（游标、last_run、monitor）；不清配置表 |
| 6012 M127 Add | 是 | 解码 array<6013 单元>，按 task_id upsert（新增或更新），落库并重排时钟 |
| 6012 M128 Delete | 是 | 解码 array<unsigned>，按 ID 删除；不存在的 ID 视为成功（幂等） |
| 6012 M129 Clear | 是 | 仅接受 NULL，清空任务表（含持久化） |
| 6012 M130 Update | 是 | {unsigned 任务 ID, enum 状态}；状态 1/2，缺失任务 DAR 6 |
| 6012 M2/M3 | 否 | 类定义未广告 → DAR 5 |
| 6012 M131+ | 否 | 类未定义 → DAR 4 |
| 6012 A2 | 是 | array<6013 单元>，按 task_id 升序 |
| 6013 A2 | 是 | 单个 12 字段单元；OAD 元素索引 = 任务 ID（0 = 最小存在 ID），缺失 → DAR 6 |
| 6013 其它属性/方法 | 否 | 类定义未广告 → DAR 5 |

## 数据形状

- `dcu_task_cfg_t`：task_id、原始 TI（unit/value）、scheme_type、scheme_id、状态、period_type；store 里是权威 RAM 镜像。
- 运行期 `dcu_task_unit_t` 内嵌 cfg，另带 clock/cursor/monitor；调度仍只有 `dcu_poller` 一处。
- TI 线格式：`0x54 | unit(1) | value_be16`（按预定义 SEQUENCE 惯例推断，编码与解码同一处；若日后有实帧不符，一处可改）。
- 不支持的合法字段（开始/结束、延时、优先级、脚本、非空时段表、方案类型 2–5、TI 月/年）以 DAR 2 拒绝并逐字段记录日志；空时段表接受并保存以便回读。
- task_id 0 保留为非法，避免与「0 = 最小 ID」寻址冲突。

## 持久化

- 新 `dcu_task_config` 表建在既有 `storage->db`；DB 先写、RAM 后改，失败回滚。
- 首次启动播种默认任务（1、5 秒、方案 1）；Clear 之后重启保持为空。
- 重启用同一连接装载并回填轮询器。

## 门禁

（见 tests.log / runtime.log）
