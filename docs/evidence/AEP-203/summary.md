# AEP-203 证据包总结 (Summary)

- **工单 ID**: AEP-203 (Issue #81)
- **标题**: 迁移 `datahub_dds` / `datahub_mqtt` → `modules/edge_infra/bus_*` 并保持 vendored fallback
- **分支**: `ticket/AEP-203`
- **状态**: Done (Ready for Review)
- **完成时间**: 2026-08-16

---

## 1. 变更内容与改动说明

1. **总线驱动源码平移与模块化**:
   - 将 `modules/edge_core/src/datahub_dds.c` 物理平移至 `modules/edge_infra/bus_dds/datahub_dds.c`；
   - 将 `modules/edge_core/src/datahub_mqtt.c` 物理平移至 `modules/edge_infra/bus_mqtt/datahub_mqtt.c`；
   - 在 `datahub_ops.h` 与 `datahub.h` 之间确立标准头文件包含与类型复用关系，解除冗余重定义。

2. **构建拓扑与 Vendored Fallback 保留**:
   - `modules/edge_infra/CMakeLists.txt` 构建 `edge_infra` 静态库，包含 `bus_dds/datahub_dds.c` 与 `bus_mqtt/datahub_mqtt.c`，链接 `cyclonedds` 与 `PahoMQTT::PahoMQTT`；
   - 保持 `third-party/CMakeLists.txt` 中 Cyclone DDS 与 Paho MQTT 的 vendored 源码 fallback 与本地动态库检测机制；
   - `modules/edge_core/CMakeLists.txt` 移除 `src/datahub_dds.c` 与 `src/datahub_mqtt.c`，直接通过 `edge_infra` 链接总线驱动。

3. **符号导出无损校验 (`nm -g`)**:
   - `nm -g` 验证：`datahub_dds_init`, `g_datahub_dds_ops`, `datahub_mqtt_init`, `g_datahub_mqtt_ops` 全部无损从 `libedge_infra.a` 导出，符号差异为 0。

4. **单测扩展与专项回归**:
   - `test_infra_interfaces.c` 扩展覆盖 DDS 与 MQTT 总线驱动的 VTable 注册、初始化、发布、读写快照与销毁；
   - `test_tinyml_overhead` 专项单独实测通过（1000 次推理平均时延 0.361ms）；
   - CTest 135/135 100% Passed (41.82s), Omnis Matrix 10/10 Passed (54.25s), Go 单元测试全绿。

---

## 2. DoD 逐项验收勾选

- [x] `datahub_dds.c` 与 `datahub_mqtt.c` 迁入 `modules/edge_infra/bus_*`
- [x] Vendored fallback 机制完整保留
- [x] 符号验证 `nm -g` 差异为 0（DDS 与 MQTT 初始化及 ops 符号全部保留）
- [x] `test_tinyml_overhead` 专项实测通过（0.361ms）
- [x] 135/135 CTest + 10/10 Omnis + Go 测试全绿
- [x] 无 R-01~R-14 红线违规
