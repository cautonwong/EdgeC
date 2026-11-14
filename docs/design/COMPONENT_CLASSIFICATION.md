# 组件分类与迁移顺序

> 状态：Accepted（2026-11-07）。依据：分层组件架构提案 §16-D、ADR-0011。
> 目录名不是规范；门禁按规则映射到现有路径。

| 单元 | 分类 | 依据 | 当前状态 | 下一个动作 |
|---|---|---|---|---|
| `foundation/` | foundation | 纯 CP56Time2a/CRC/端口类型/64B PipeMsg/命令与仲裁值类型/contract 数据类型/registry/common/request/TLV/port/version/驱动接口、驱动会话端口与驱动注册端口；`edge_netbuf` 缓冲池、`edge_crypto` ops 注册表、`edge_codec` 纯字节 codec/UNS engine、`edge_math` 矩阵/DSP/控制算法、`edge_log` 日志实现 | 已建立；`net_buf`/`hal_crypto`/全部纯协议 codec/纯数学 DSP/PipeMsg/arbiter 值类型/日志实现/contract 纯类型/registry/common/request/TLV/port/version 已下沉 | 继续迁入无 IO 的纯算法 |
| `modules/edge_core` | infra | DataHub、SHM、运行时 | 被 8 个产品链接；contract/registry/common/request/TLV/port/version 纯头已下沉 foundation；端口适配器已迁至 edge_bootstrap；`edge_service` 不再持有或启动北向管理器，也不再持有 SQLite 存储或导出历史数据；`edge_params` 经 `edge_params_store_t` 持久化，核心已无 SQLite 引用 | 把剩余纯值类型抽到 foundation；DataHub 只经端口暴露 |
| `modules/edge_infra` | infra | SQLite、DDS、网络、存储、防篡改 | 稳定；安全机制已拆到 edge_infra_security，且不再 PUBLIC 转发；NPU 与 DDS 拓扑写遥测改走 `edge_datahub_write_port_t` | 保持不反向依赖组件 |
| `modules/edge_infra_security` | infra | license、ESAM、SOS、DPI 包检测 | 独立 infra 模块；iems/测试显式链接，DPI 通过 edge_io guard port 注入；DPI 初始化不再接收 DataHub | 保持机制与策略分离 |
| `modules/edge_bootstrap` | infra / 组合支持 | 组合根装配 | 持有 `edge_driver_runtime`（manager 存储、注册端口、会话端口），其余 DataHub 桥、SQLite、端口适配器均已独立到 `adapters/` | 保持组合根只做生命周期与装配 |
| `adapters/mcp_industry` | adapter | AEP-508 行业 MCP 工具与微服务 Provider | 独立 `edge_mcp_industry` 库，产品按需链接 | 保持工具包按需注册 |
| `adapters/rest_api` | adapter | 管理面 HTTP/REST 与 web server | `rest_server`/`web_server` 从 edge_io 移出，独立 `edge_rest_api` 库；iEMS 与测试按需链接 | 保持传输适配器不进入领域组件 |
| `adapters/datahub_bridges` | adapter | DataHub 与流、协议桥、北向、ADC/雷达、bus 的桥接 | 从 edge_bootstrap 移出，独立 `edge_datahub_bridges` 库；组合根与测试按需链接 | 保持桥接适配器不进入组件 |
| `adapters/sqlite_adapters` | adapter | app repo、stream、SnF、params、persistence 的 SQLite 实现 | 从 edge_bootstrap/edge_io 移出，独立 `edge_sqlite_adapters` 库 | 保持持久化适配器不进入组件 |
| `adapters/edge_ports` | adapter | consumer port 的标准胶水与 mock 实现 | 从 edge_bootstrap 移出，独立 `edge_port_adapters` 库 | 保持端口实现不进入组合根 |
| `adapters/linux_platform` | adapter | Linux io_executor、TSN、BSP HAL | 从 edge_io 移出，独立 `edge_linux_platform` 库；edge_io 链接该实现 | 其他平台按同一接口提供实现 |
| `adapters/io_drivers` | adapter | 南向协议驱动实现与驱动管理器 | 驱动源和 `driver_manager` 已从 edge_io 迁出为 `edge_io_drivers` STATIC 库；manager 用调用方存储创建，无堆工厂；`edge_driver_session_port_t` 和 `edge_driver_registry_port_t` 分别暴露生命周期与注册；产品只依赖端口，产品与测试显式链接本适配器 | 保持端口适配器按需链接 |
| `modules/edge_protocol` | 已删除 | 纯 codec、converter 与 UNS engine 已并入 `foundation/edge_codec` | 模块不再存在；Kconfig 迁至 `foundation/Kconfig` | 无 |
| `modules/edge_security` | 已删除 | 认证/密钥机制在 edge_infra，网络 ACL 策略在 edge_security_policy | 兼容聚合目标已移除，消费者直接链接 edge_infra/edge_security_policy | 无 |
| `modules/edge_security_policy` | component | 网络访问控制策略（ACL） | 独立组件，仅依赖 foundation；按源 IP/掩码、端口、协议匹配 ALLOW/DROP | 策略经端口暴露 |
| `modules/edge_session` | component（支撑子域） | 从站会话管理与驱动实例生命周期 | 从 edge_io 独立，`edge_slave_plane` 端口下沉 foundation；驱动生命周期经 `edge_driver_session_port_t` 注入，不再链接 edge_io_drivers | 保持会话不直接取数，也不依赖驱动实现 |
| `modules/edge_api` | component（支撑子域） | 产品级读/写/原始 IO 门面 | 从 edge_io 独立，初始化只接收 bus_orchestrator，不再持有或接收 driver manager；产品显式链接 | 保持门面只做参数校验与转发 |
| `modules/edge_north` | component（支撑子域） | 北向协议管理与 SnF | 从 `edge_io` 独立：`northbound_mgr` 与 `snf_manager` 归本模块；MQTT 传输在 `adapters/mqtt_transport`；SnF、DataHub 数据源和北向驱动均经端口注入，不再依赖 edge_core/edge_infra/io_drivers | 保持端口适配器在 bootstrap |
| `modules/edge_io` | 拆分：infra + component | 编排/引擎属 infra，经 `edge_bus_datahub_port_t` 访问 DataHub；REST/web、MQTT agent、Linux 执行器分别在 adapters；驱动与协议桥属 component；公开 API 门面在 edge_api；会话在 edge_session；SQLite persistence 已移出 | 驱动实现与 `driver_manager` 已物理迁至 `adapters/io_drivers`；edge_io/edge_session 通过 `edge_driver_session_port_t` 使用驱动生命周期，不再链接驱动适配器；安全校验改走 `edge_io_guard_port_t`，REST 服务已移出 | 保持适配器由产品显式链接 |
| `modules/edge_stream` | component | 流计算与算子 | datahub 引用 0；公共头不再 include `shmring.h`/`edge_port.h`/`arbiter.h`；直接链接 `edge_log`/`edge_math`，不再 include 或链接 edge_algo/edge_core/edge_infra；算法流适配器移至 `adapters/stream_algo` | 补 `component.yaml` 预算度量 |
| `modules/edge_ai` | component（核心子域） | AI 推理、量化、张量池与 PINN 算法 | 从 edge_algo 独立；`edge_algo_port.h` 下沉 foundation；`edge_algo` 通过 PUBLIC 链接复用 | 保持 AI 算法不反向依赖业务组件 |
| `modules/edge_battery` | component（核心子域） | 电池建模、SOH/EIS、均衡、热管理与策略 | 从 edge_algo 独立；仅依赖 foundation/edge_math；edge_algo 聚合链接 | 保持电池算法不反向依赖总线/存储 |
| `modules/edge_health` | component（核心子域） | 健康监测、康复、ECG/PPG/EMG、步态与跌倒检测 | 从 edge_algo 独立；仅依赖 foundation/edge_math；edge_algo 聚合链接 | 保持健康算法不反向依赖业务组件 |
| `modules/edge_power` | component（核心子域） | VSG、FFR、黑启动、预同步与场站微网协调 | 从 edge_algo 独立；仅依赖 foundation/edge_math；edge_algo 聚合链接 | 保持电力电子算法不反向依赖总线/存储 |
| `modules/edge_runtime` | component（核心子域） | CEP、WASM NN、算子融合、ONNX host guard、联邦聚合与通用算法运行时 | 从 edge_algo 独立；仅依赖 foundation/edge_math；edge_algo 聚合链接 | 保持运行时算法不反向依赖业务组件 |
| `modules/edge_mgmt` | component | 应用与配置管理 | `app_repo` 端口完成，sqlite 适配器在 edge_bootstrap；REST 经 `rest_control_port_t` 调用；app 日志经组件自有 logger 端口，无 edge_core 链接 | 配置管理经端口暴露；app 聚合根只保留生命周期不变量 |
| `modules/edge_mcp` | component | MCP 服务与工具分发 | `edge_mcp_tool_port_t` 完成；dispatcher 只保留通用 DataHub 工具，行业算法工具/Provider 包已迁至 `adapters/mcp_industry`，由产品按需注册；无 edge_core include/link | 保持工具包按需注册 |
| `cmd/dcu`、`cmd/iems`、`cmd/smart_box` | product / 组合根 | 已进构建的交付物 | 三个根 | 继续瘦身，保持预算账本只减不增 |
| `cmd/edge_mcp_server`、`cmd/nexus_lab` | product / 组合根 | 已进构建的交付物 | 两个根 | 明确交付边界 |
| `cmd/evse`、`cmd/hems`、`cmd/sg` | 已删除 | 未进构建，且依赖已删的 `contracts.h` | 不计交付物 | 已按本行结论删除 |

**点表。** ADR-0010 规定点表由产品防腐层用极简静态 C 结构体维护，不引入 JSON/YAML 配置层；manifest 不作为运行时输入。

**组件元数据。** `component.meta` 使用 key=value，不使用 JSON/YAML。所有 `modules/*` 和 `adapters/*` 必须提供 `component.meta`，缺失即门禁失败。

**DataHub。** 组件文件禁止 include `datahub.h`；infra 文件允许持有 DataHub（ADR-0011）。

**管理面端口。** `edge_io` 的 REST 服务器只 include `rest_control_port.h`，不 include `edge_app_mgr.h`、`stream_engine.h`；iEMS 的 `iems_app_manager_adapter` 以 app 管理器与流引擎实现该端口（app 列表/启停/安装/拓扑），组合根通过 `service_set_control_port` 注入。

**MCP 工具端口。** `edge_mcp` 只定义 `read_tag` / `write_tag` / `get_tag_meta` 的消费者端口与组件自有值类型；DataHub 适配器在 `edge_bootstrap`。没有端口的工具不再返回常量演示数据，而是返回明确错误；产品特定工具由组合根按需注册。行业算法工具与微服务 Provider 注册已迁至 `edge_bootstrap/src/mcp_industry/`，dispatcher 创建后默认只有通用工具，`cmd/iems` 与 `cmd/edge_mcp_server` 通过 `edge_mcp_register_industry_tools` / `edge_mcp_register_all_microservices` 显式启用。

**目录。** 目录名不是规范；门禁按文件映射，物理重排不是完成条件（提案 §11.4）。

**规则。** 组件只依赖 foundation、自身端口和白名单内的纯第三方库；infra 不认识组件；组合根是唯一同时认识组件与 infra 的代码。执行类端口只注入持有对应不变量的聚合根。分类变化必须更新本文件和相应的门禁基线。

**成熟度晋升。** `incubating` 升 `stable` 必须同时满足：真实 ROM/RAM 预算、独立构建目标、组件自有测试、`ports_required` 与 include/链接一致、至少一个产品在用。`stable` 组件需要 `backup_owner`，改动按双评审处理。`deprecated` 只保留迁移窗口。

**领域与结构体。** 三个产品的限界上下文、聚合、仓储、领域事件和端口映射见 `docs/design/DOMAIN_CONTEXT_MAP.md`。公共结构体逐个审计见 `docs/design/PUBLIC_STRUCT_AUDIT.md`；新增公共 struct 必须同时更新审计文档和基线。
