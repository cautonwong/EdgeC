# 组件分类与迁移顺序

> 状态：Accepted（2026-11-07）。依据：分层组件架构提案 §16-D、ADR-0011。
> 目录名不是规范；门禁按规则映射到现有路径。

| 单元 | 分类 | 依据 | 当前状态 | 下一个动作 |
|---|---|---|---|---|
| `foundation/` | foundation | 纯 CP56Time2a 编解码，仅标准库 | 已建立 | 继续迁入无 IO 的纯 codec |
| `modules/edge_core` | infra | DataHub、SHM、运行时、共享值类型 | 被 8 个产品链接 | 把纯值类型抽到 foundation；DataHub 只经端口暴露 |
| `modules/edge_infra` | infra | SQLite、DDS、网络、存储 | 稳定 | 保持不反向依赖组件 |
| `modules/edge_bootstrap` | infra / 组合支持 | 组合根装配、端口适配器 | 新增 `stream_bus_datahub_port` | 适配器集中到此或独立 `adapters/` |
| `modules/edge_protocol` | 拆分：foundation + component | 字节 codec 纯代码；协议业务语义属组件 | 仍是单一模块 | 逐个把纯 codec 搬入 foundation |
| `modules/edge_security` | 拆分：infra + component | 认证/密钥机制属 infra；授权策略属组件 | 仍是单一模块 | 先拆机制与策略，再定义策略端口 |
| `modules/edge_io` | component | 南向编排与驱动 | 允许依赖 7 个模块 | 先拆驱动注册与 arbiter，再按端口收敛 |
| `modules/edge_stream` | component | 流计算与算子 | 试点完成，datahub 引用 0 | 补 `component.yaml` 预算度量 |
| `modules/edge_algo` | component（核心子域） | 算法与控制策略 | 依赖 edge_core | 纯算法函数优先下沉 foundation |
| `modules/edge_mgmt` | component | 应用与配置管理 | 依赖 edge_core/edge_infra | 定义管理端口 |
| `modules/edge_mcp` | component | MCP 服务与工具分发 | 依赖 4 个模块 | 定义工具端口 |
| `cmd/dcu`、`cmd/iems`、`cmd/smart_box` | product / 组合根 | 已进构建的交付物 | 三个根 | 继续瘦身，保持预算账本只减不增 |
| `cmd/edge_mcp_server`、`cmd/nexus_lab` | product / 组合根 | 已进构建的交付物 | 两个根 | 明确交付边界 |
| `cmd/evse`、`cmd/hems`、`cmd/sg` | 未交付的桩 | 只有 `main.c`，未进构建 | 不计交付物 | 要么接入构建，要么删除 |

**点表。** ADR-0010 规定点表由产品防腐层用极简静态 C 结构体维护，不引入 JSON/YAML 配置层；manifest 不作为运行时输入。

**规则。** 组件只依赖 foundation、自身端口和白名单内的纯第三方库；infra 不认识组件；组合根是唯一同时认识组件与 infra 的代码。执行类端口只注入持有对应不变量的聚合根。分类变化必须更新本文件和相应的门禁基线。
