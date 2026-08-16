# AEP-207 证据包总结 (Summary)

- **工单 ID**: AEP-207 (Issue #85)
- **标题**: Kconfig 重排（`CONFIG_EDGE_INFRA_SQLITE/DDS/MQTT`）与三产品独立编译解耦
- **分支**: `ticket/AEP-207`
- **状态**: Done (Ready for Review)
- **完成时间**: 2026-08-16

---

## 1. 变更内容与改动说明

1. **`edge_infra` Kconfig 选项重排与解耦 (`modules/edge_infra/Kconfig`)**:
   - 编写落盘 [modules/edge_infra/Kconfig](file:///workspaces/edgeos/EdgeC/modules/edge_infra/Kconfig)，定义核心基础设施开关：
     - `CONFIG_EDGE_INFRA_SQLITE`: SQLite 存储服务与驱动编译开关
     - `CONFIG_EDGE_INFRA_DDS`: CycloneDDS 分布式总线后端编译开关
     - `CONFIG_EDGE_INFRA_MQTT`: MQTT 北向云端网桥后端编译开关
   - 在根目录 [Kconfig](file:///workspaces/edgeos/EdgeC/Kconfig) 中 source 引入 `modules/edge_infra/Kconfig`。

2. **`modules/edge_infra/CMakeLists.txt` 条件编译与链接隔离**:
   - 重构 [modules/edge_infra/CMakeLists.txt](file:///workspaces/edgeos/EdgeC/modules/edge_infra/CMakeLists.txt)，依据 Kconfig 选项动态收集源码文件与依赖库：
     - 启用 `CONFIG_EDGE_INFRA_SQLITE` 时编译 `storage/sqlite_service.c`、`storage/sqlite_storage_driver.c` 并链接 `SQLite3`；
     - 启用 `CONFIG_EDGE_INFRA_DDS` 时编译 `bus_dds/datahub_dds.c` 并链接 `cyclonedds`；
     - 启用 `CONFIG_EDGE_INFRA_MQTT` 时编译 `bus_mqtt/datahub_mqtt.c` 并链接 `PahoMQTT`。

3. **三产品（IEMS / DCU / SmartBox）配置与独立编译支持**:
   - 在 [cmd/iems/prj.conf](file:///workspaces/edgeos/EdgeC/cmd/iems/prj.conf)、[cmd/dcu/prj.conf](file:///workspaces/edgeos/EdgeC/cmd/dcu/prj.conf)、[cmd/smart_box/prj.conf](file:///workspaces/edgeos/EdgeC/cmd/smart_box/prj.conf) 中显式声明各自基础设施依赖；
   - 验证 `ninja iems`、`ninja dcu`、`ninja smart_box` 三产品目标独立编译全部成功。

4. **闭合 AEP-205 的 3 条尺寸门禁加固建议**:
   - **建议 1（自校验 Release）**：在 [scripts/check_binary_sizes.sh](file:///workspaces/edgeos/EdgeC/scripts/check_binary_sizes.sh) 中增加构建模式检测与自校验输出；
   - **建议 2（缺失产物报 ERROR）**：在尺寸门禁中对核心库及必需产物执行存在性强制检查，缺失即报 ERROR 退出；
   - **建议 3（上传尺寸报告）**：在 [.github/workflows/ci.yml](file:///workspaces/edgeos/EdgeC/.github/workflows/ci.yml) 中通过 `tee build/size_report.txt` 生成报告并加入 CI artifact 上传。

5. **全量测试与门禁验证**:
   - `ctest --test-dir build -j1`: **136/136 100% Passed (61.19s)**；
   - `./test.sh --omnis`: **10/10 Passed (58.74s)**；
   - `go test ./...`: 100% Passed；
   - 5 大反熵架构门禁全绿（`check_no_old_src.sh`, `check_symbols_baseline.sh`, `check_core_symbols.sh`, `check_core_readonly.sh`, `check_binary_sizes.sh`）。

---

## 2. DoD 逐项验收勾选

- [x] `modules/edge_infra/Kconfig` 落盘并定义 `CONFIG_EDGE_INFRA_SQLITE/DDS/MQTT`
- [x] `modules/edge_infra/CMakeLists.txt` 实现基于 Kconfig 的条件编译与依赖隔离
- [x] iems、dcu、smart_box 三产品 `prj.conf` 同步并完成独立编译验证
- [x] 尺寸脚本增加 Release 模式自校验
- [x] 尺寸脚本对缺失必需产物报 ERROR
- [x] CI 工作流上传 `build/size_report.txt`
- [x] 136/136 CTest -j1 + 10/10 Omnis + Go 测试全绿
- [x] 真实证据日志入库
