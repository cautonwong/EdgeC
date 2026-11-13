# 领域上下文图与聚合清单

状态：2026-11-13。依据：`cmd/iems/CONTEXT.md`、`cmd/dcu/CONTEXT.md`、
`cmd/smart_box/CONTEXT.md`、`docs/design/COMPONENT_CLASSIFICATION.md`、ADR-0008/0010/0011，
以及当前代码里的仓储、端口和事件落点。

## 1. 上下文图

| 限界上下文 | 产品 | 类型 | 代码落点 | 聚合根 |
|---|---|---|---|---|
| 储能控制 | iEMS | core | `cmd/iems`、`modules/edge_power`、`modules/edge_ai` | `ESSContainer` |
| 应用平台 | iEMS | supporting | `modules/edge_mgmt`、`modules/edge_app_host`、`modules/edge_bootstrap` | `App`、`RegistrySnapshot` |
| 遥测与总线 | iEMS/DCU/SmartBox | supporting（共享内核） | `modules/edge_core`、`modules/edge_infra`、`modules/edge_io`、`modules/edge_session` | 无聚合；`DataHub` 是共享内核，`DeviceSession` 是实体 |
| 流分析 | iEMS/DCU/SmartBox | core/supporting | `modules/edge_stream` | `Pipeline` |
| 北向与 SnF | 三个产品 | supporting | `modules/edge_north`、`adapters/mqtt_transport` | `Subscription`、`SnfRecord` |
| 访问控制 | 三个产品 | supporting | `modules/edge_mgmt`（grant/secret）、`modules/edge_security_policy` | `Grant`、`Policy` |
| 台区与线损 | DCU | core | `cmd/dcu` | `DistributionArea` |
| 四遥测控 | DCU | core | `cmd/dcu` | `TelecontrolUnit` |
| 停电与 SOE | DCU | core | `cmd/dcu` | `OutageDetector`、`SOERecord` |
| 智能配电箱 | SmartBox | core | `cmd/smart_box` | `SmartPanel` |
| 智能微断 | SmartBox | core | `cmd/smart_box` | `CircuitBreaker` |
| 操作授权 | SmartBox | supporting | `cmd/smart_box`（双授权流程）、`web`（确认 UI） | `ControlOperation`（领域命名待确认） |

上下文关系：

- 三个产品共享遥测与总线、流分析、北向、访问控制四个 supporting 上下文。
- `edge_core` 与 `edge_infra` 组成共享内核；组件不 include `datahub.h`，infra 才持有 DataHub。
- 产品上下文通过端口使用共享内核，组合根是唯一同时认识组件与 infra 的代码。
- 应用平台是 iEMS 的首个上下文；DCU/SmartBox 目前只用到它的状态展示和任务台账部分。

## 2. 聚合与不变量

### iEMS

| 聚合根 | 实体/值对象 | 不变量 |
|---|---|---|
| `ESSContainer` | `bms`、`pcs`、`grid_meter`、`BatteryEnvelope`、`PCSMode` | SOC 运行窗口、防逆流阈值、变压器需量上限、PCS 模式约束 |
| `App` | manifest、grant、点位槽位、运行状态 | 状态机 `UNINSTALLED → INSTALLED → RUNNING/STOPPED/FAULTED`；`desired_state` 与实态分离；10 分钟故障窗口内最多 5 次重启；manifest 按版本不可变 |
| `RegistrySnapshot` | generation、apps、consumers | generation 单调递增；只有全部消费者新鲜且追平才算 `converged` |
| `Grant` | capability、scope、purpose、expiry、revocation、publisher trust | 敏感度与 purpose、发布者信任上限、升级继承窄化 |
| `Pipeline` | stage 图、触发器、状态 | DAG 无环、stage 顺序、背压上限 |

### DCU

| 聚合根 | 实体/值对象 | 不变量 |
|---|---|---|
| `DistributionArea` | `transformer`、`branches[16]` | 线损 = 供入 − 供出；分支拓扑与相位映射唯一 |
| `TelecontrolUnit` | YX/YK/YC/YT | 遥控双确认与闭锁；遥信防抖；遥调档位范围 |
| `OutageDetector` / `SOERecord` | 事件类型、微秒时间戳、前后快照 | 停电捕获不丢；SOE 顺序与快照一致 |

### SmartBox

| 聚合根 | 实体/值对象 | 不变量 |
|---|---|---|
| `SmartPanel` | `breakers[32]`、`env_sensor` | 火灾风险阈值；批量控制联动顺序 |
| `CircuitBreaker` | 状态、电流、漏电流、端子温度、电能 | 状态机 `分闸/合闸/故障跳闸/就地闭锁`；漏电与温度越限触发脱扣 |
| `ControlOperation` | 操作人、复核人、目标、结果 | 双授权未完成不得下发；同一操作只落一次 |

## 3. 仓储清单

| 仓储用途 | 端口 | 适配器/存储 |
|---|---|---|
| 应用注册表 | `edge_app_repo_t` | `app_repo_sqlite` |
| 应用点位槽位 | `app_points_sqlite` 系列 | `app_points_sqlite` |
| 授权与发布者信任 | `edge_authz_repo_t`（读）、`edge_authz_store_t`（写） | `edge_authz_sqlite` |
| 应用密钥 | `edge_secret_store_t` | `edge_secret_sqlite` |
| 轻量任务台账 | `edge_app_task_store_t` | `edge_app_task_sqlite` |
| 运行参数 | `edge_params_store_t` | `edge_params_sqlite_adapter` |
| 北向 SnF | `snf_store_port_t` | `snf_sqlite_adapter` |
| 流水线配置与状态 | `pipeline_config_port_t`、`stream_sqlite_port_t` | SQLite 适配器 |
| 遥测历史 | `edge_telemetry_store_t` | `edge_infra` 存储实现 |
| 注册表快照 | 文件发布，不是仓储 | `edge_registry_snapshot_publish()` 生成 `gen-<N>.json` |

## 4. 领域事件

| 上下文 | 事件 | 代码落点 |
|---|---|---|
| 应用平台 | installed/started/stopped/faulted/restarted/uninstalled/upgraded/rolled back | `edge_app_mgr`、`edge_app_mgr_reconcile` |
| 应用平台 | task finished、export/purge | `edge_app_task`、卸载回调 |
| 注册表 | snapshot published、consumer converged/pending | `edge_registry_snapshot_publish`、`consumer_heartbeat` |
| 访问控制 | grant approved/revoked/inherited、publisher trust recorded | `edge_authz` |
| 时间 | trust changed（TRUSTED/DEGRADED/UNTRUSTED） | `edge_time_trust`、`/api/v1/system/time` |
| 流分析 | pipeline loaded/replaced、stage fault | `stream_engine` |
| DCU | outage detected、SOE recorded、topology identified、line loss abnormal | `dcu_outage_detector`、`dcu_event_monitor`、`dcu_inventory` |
| SmartBox | fire risk warning、breaker operation、double auth approved | `cmd/smart_box`、双授权流程 |

标为“领域事件”的条目是聚合状态变化的对外事实；没有事件总线的部分由仓储写入和
REST/快照回调承担通知，后续要加事件总线时从这里取清单。

## 5. 端口到聚合的映射

| 端口 | 定义位置 | 对应聚合/上下文 | 说明 |
|---|---|---|---|
| `edge_app_repo_t` | `modules/edge_mgmt/include/edgeos/edge_app_mgr.h` | `App` | 应用注册表仓储 |
| `rest_control_port_t` | `modules/edge_io/include/rest_control_port.h` | `App`、`RegistrySnapshot` | 管理面外观，不持有不变量 |
| `edge_mcp_tool_port_t` | `modules/edge_mcp/include/edgeos/edge_mcp_tool_port.h` | DataHub 遥测 | 防腐层；AI 工具经它读写点位，不直接碰聚合 |
| `app_host_ports_t` | `modules/edge_app_host/include/edge_app_host.h` | `App` | 应用运行时边界；宿主只暴露 point/log/now |
| `edge_bus_datahub_port_t` | `foundation/include/edge_bus_datahub_port.h` | DataHub | 总线侧的发布端口 |
| `edge_datahub_write_port_t` | `foundation/include/edge_datahub_write_port.h` | DataHub | 只写影子的生产者端口 |
| `stream_bus_port_t` | `modules/edge_stream/include/stream_engine.h` | DataHub | 流引擎输入输出 |
| `nb_source_ops_t` | `modules/edge_north/include/northbound_mgr.h` | 遥测/事件源 | 北向读取源，输出端口 |
| `snf_store_port_t` | `modules/edge_north/include/snf_manager.h` | `SnfRecord` | SnF 持久化 |
| `edge_driver_session_port_t` | `foundation/include` | `DeviceSession` | 驱动生命周期 |
| `edge_driver_registry_port_t` | `foundation/include` | `DeviceSession` | 驱动注册 |
| `edge_params_store_t` | `modules/edge_core/include/edge_params.h` | 参数配置 | 配置上下文仓储 |
| `pipeline_config_port_t` | `modules/edge_stream/include/stream_engine.h` | `Pipeline` | 流水线配置 |
| `stream_sqlite_port_t` | `modules/edge_stream/include/stream_engine.h` | `Pipeline` | 流水线状态与快照 |

## 6. 边界规则

1. 聚合根持有不变量；REST、MCP、CLI、快照回调都是外观或防腐层，不改聚合状态。
2. 组件只依赖 foundation、自身端口和白名单第三方库；infra 不认识组件；组合根负责装配。
3. 组件不得 include `datahub.h`，DataHub 只经端口暴露。
4. 仓储端口由消费者定义，适配器放 `adapters/`；聚合不直接依赖 SQLite。
5. 新增上下文或聚合必须同时更新本文件和 `PUBLIC_STRUCT_AUDIT.md` 的相关条目。

## 7. 待确认

- SmartBox 的 `ControlOperation` 是否是一个独立聚合，还是 `SmartPanel` 内的操作实体。
- DCU 的线损与拓扑是否共用 `DistributionArea` 聚合，还是拆成两个聚合。
- `RegistrySnapshot` 的 generation 目前由文件发布和仓储代次共同维护，是否要并入 `App` 聚合。
