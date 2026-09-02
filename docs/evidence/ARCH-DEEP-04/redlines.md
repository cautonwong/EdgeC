# ARCH-DEEP-04 纪律与架构红线自检清单 (Redlines)

| 红线检查项 | 约束标准 | 达标情况 | 证明依据 / 备注 |
|---|---|:---:|---|
| **Boss Protocol** | 每轮以 Mr. WKT 开头 | ✅ PASS | 全程强制遵循上下文焦点对齐 |
| **声明式 DAG 契约装配** | YAML/JSON 纯配置驱动流计算拓扑自动装配与连线 | ✅ PASS | 实现 `stream_engine_load_yaml_contract`, `stream_engine_load_yaml_file` |
| **卡恩算法拓扑防环校验** | 加载期自动检测并拒绝环路依赖拓扑 ($A \to B \to A$) | ✅ PASS | 实现 `dag_validate_acyclic`，单测实测返回 `-22` 并安全回收资源 |
| **开闭原则与动态工厂扩展** | 算子插件化动态注册，无需修改核心 C 源码 | ✅ PASS | 实现 `stream_engine_register_stage_factory`，实测自定义算子即插即用 |
| **Graphviz 可视化导出** | 一键导出当前运行拓扑的 DOT 结构 | ✅ PASS | 实现 `stream_engine_export_dot`，支持 Web UI / 前端图表可视化 |
| **物理真实性 (Physical Truth)** | 无 Mock 占位符、真实 DAG 拓扑执行 | ✅ PASS | 编写 `tests/test_stream_yaml_assembly.c`（3/3 测试通过） |
| **手术级修改 (Surgical Edit)** | 仅改动目标计算平面模块 | ✅ PASS | 仅深化 `modules/edge_stream/src/pipeline_loader.c` 与 SDK 头文件 |
| **反熵核心只读门禁** | `STRICT_CORE_GATE=1 check_core_readonly.sh` | ✅ PASS | 0 违规核心修改，核心只读门禁通过 |
| **反熵符号基线门禁** | `STRICT_CORE_GATE=1 check_core_symbols.sh` | ✅ PASS | `libedge_core.a` 0 外部禁止符号 |
| **二进制尺寸门禁** | `STRICT_SIZE_GATE=1 check_binary_sizes.sh` | ✅ PASS | 4/4 产品二进制严控尺寸预算通过 |
| **生成物可信链门禁** | `STRICT_DRIFT_GATE=1 check_generated_drift.sh` | ✅ PASS | 编译器一致(delta:0B)，全生成物零漂移 |
| **独立分支与证据包** | ticket/ARCH-DEEP-04, summary/tests/redlines/diffstat | ✅ PASS | 独立分支 `ticket/ARCH-DEEP-04`，全量 `tests.log` 实测入库 |
