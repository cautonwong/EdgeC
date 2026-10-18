# EdgeOS / EdgeC Contract Directory (Deprecated / Migrated to Autonomous Aggregates)

> [!NOTE]
> 根据 **ADR-0010** 与 **[No-TagID, No-Contract 架构白皮书](../docs/design/no-tagid-no-contract-architecture.md)**，
> 全局中心化契约编译器 (`contract-compiler-v2`)、YAML 模式点表与数字 TagID 系统已被彻底废除。
> 
> 各产品（DCU、IEMS、SmartBox）已全面演进为自治聚合根（`dcu_node_t`, `iems_node_t`, `smartbox_node_t`），
> 测点使用领域天然标识并通过防腐层（ACL）驱动适配器进行依赖注入。
>
> 当前目录仅保留 `idl/` 子目录，用于 Cyclone DDS 工业标准化通讯（DL/T 698.45 / DLMS Bluebook / IDIS Pack3 OMG IDL 4.2）结构体定义。
