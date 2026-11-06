# DataHub 归 infra，组件只通过端口访问

DataHub 是跨模块共享的可变状态，有 IO、有全局生命周期，不满足 foundation 的准入条件。决定把它归为 **infra**，由组合根创建、持有和销毁；组件不得直接引用 `datahub.h`，只能通过自己定义的端口（例如 `stream_bus_port_t`）或领域事件访问。`edge_stream` 是第一个完成迁移的组件，`check_component_boundary.py` 阻止 `datahub.h` 回流。

**后果。** `CONTEXT-MAP.md` 中的 “Shared Kernel” 改为 “Shared Runtime”；DataHub 不是共享内核模型，而是 infra 提供的运行时。适配器放在组合根一侧，是唯一同时认识 DataHub 与组件的代码。后续组件迁移沿用同一模式：先定义端口，再迁移调用方，最后删除旧路径。
