通过对全量代码的深度解构，我为您制定了 Sprint 4 "Omnis" (万物一体)
架构落地的最优执行方案。

核心哲学：环境跟着契约走 (The Environment Follows the Contract)
我们要彻底废弃 world.yaml，将 “程序逻辑” 与 “物理/模拟环境” 统一在 extension.yaml 中。

---

🚀 Sprint 4 "Omnis" 详尽 Backlog (研发任务清单)

第一阶段：契约编译器 (Contract Compiler) 升维
目标：让编译器具备解析硬件拓扑和处理管线的能力，生成“自初始化”元数据。

┌────┬────┬────────────────────────────────────────────────┬────────────┬──────────────
──┐
│ 任务 │ 模块 │ 任务描述 │ 涉及文件 │ 交付物
│
├────┼────┼────────────────────────────────────────────────┼────────────┼──────────────
──┤
│ ** │ ** │ 在 Go 模型中定义 HardwareDef 和 PipelineDef... │ `model/... │
  强类型数据结构 │
  │ ** │ ** │ 增强合并逻辑：允许 extension.yaml 完全覆盖...  │ `core/m... │ 唯一的
`Eff... │
  │ ** │ ** │ 编写 meta_hw.tmpl 和 meta_pipelines.tmpl，生...  │ `templa... │
`g_hw_manifes... │
  │ ** │ ** │ 升级 omnis_manifest.tmpl，导出包含 `simulat... │ templates/ │
`omnis_mani... │
└────┴────┴────────────────────────────────────────────────┴────────────┴──────────────
──┘

第二阶段：Matrix 仿真矩阵 (Matrix Server) 自动化
目标：让仿真服务器成为契约的“执行者”，不再需要手动配置世界。

┌─────┬──────┬────────────────────────────────────────────────┬─────────────┬──────────
─┐
│ 任务 │ 模块 │ 任务描述 │ 涉及文件 │ 交付物
│
├─────┼──────┼────────────────────────────────────────────────┼─────────────┼──────────
─┤
│ **M │ \*... │ 移除对 world.yaml 的依赖，改为解析编译器生... │ `engine/... │ 契约化...
│
│ **M │ Orch │ 实现根据 simulation 字段自动拉起对应的 `act... │ `engine/... │ 自动化...
│
│ \*_M │ _... │ 统一 UDS 路径管理：根据契约中的 `profiles.s... │ `engine/... │ 物理链...
│
└─────┴──────┴────────────────────────────────────────────────┴─────────────┴──────────
─┘

第三阶段：内核零代码启动 (Core Zero-Code Boot)
目标：简化 main.c，实现基于元数据的自动初始化。

┌─────┬────┬─────────────────────────────────────────────────┬───────────┬─────────────
─┐
│ 任务 │ 模块 │ 任务描述 │ 涉及文件 │ 交付物
│
├─────┼────┼─────────────────────────────────────────────────┼───────────┼─────────────
─┤
│ **C │ ** │ 遍历 g_hw_manifest：自动调用 `bus_orchestrator_s... │ `cmd/i... │ < 100
行... │
│ **C │ ** │ 实现 stream_engine_load_from_contract()，代替 JS... │ `src/s... │
  编译期确...  │
  │ **C │ ** │ [核心] 实现多模态电能积分算子，支持 `GRID_PO... │ `src/s... │ `ENERGY_I...
│
└─────┴────┴─────────────────────────────────────────────────┴───────────┴─────────────
─┘

---

📋 关键 Schema 设计 ( extension.yaml 示例)

    1 # cmd/iems/extension.yaml 终极形态
    2 hardware:
    3   - id: 0
    4     type: "BUS_SERIAL"
    5     label: "Main_Bus"
    6     profiles:
    7       simulation: { os_dev: "UDS:/tmp/vbus0.sock" } # Matrix 看这里
    8       production: { os_dev: "/dev/ttyS1" }         # C 内核看这里
    9     topology:

10 - dev_id: "METER_1"
11 address: 1
12 protocol: "ModbusRTU"
13 simulation: { actor: "modbus_actor", init: { 1001: 220.0 } } # 仿真参数
14
15 pipelines:
16 - id: "energy_logic"
17 triggers: [ {tag: "GRID_POWER"}, {event: "SYS_POWER_OFF"} ]
18 stages:
19 - type: "ENERGY_INTEGRATOR"
20 config: { ctx_tag: "ENERGY_STATE_BLOB" } # 状态锚点：ctx 即测点
21 - type: "SQLITE_SINK"
22 config: { table: "billing" }

🎯 为什么这么做最优？

1.  对于 Matrix (Python 侧)：
    它变成了一个透明的“硬件映射层”。只要契约里说 Bus 0
    是模拟的，它就拉起模拟器。它不再关心业务逻辑，只关心如何喂数据。
2.  对于内核 (C 侧)：
    main.c 变成了一个纯粹的“服务容器”。启动逻辑从 main.c 转移到了编译器。如果你在 YAML
    里加了一根总线，编译器会自动生成初始化代码，内核无需改动。
3.  对于电能模块 ( ctx 共享场景)：
    通过 ENERGY_STATE_BLOB 将上下文提升为测点，利用 DataHub
    现有的“脏数据检查”和“发布订阅”机制，完美解决了多源触发下的数据一致性问题。

下一步行动建议：
如果您认可这个 Backlog，我建议从 CC-01 (编译器模型更新) 开始，这是整个 Omnis
架构的“数据宪法”。您准备好了吗？
