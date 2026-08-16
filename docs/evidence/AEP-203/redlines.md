# AEP-203 红线自查表 (Redlines Check)

| 红线编号 | 红线要求 | 判定 | 说明 |
|---|---|:---:|---|
| R-01 | 未冻结基线不得启动 Phase 1+ | ✅ | Phase 0+1 已闭环 |
| R-02 | 物理真实性，禁止伪造数据 | ✅ | 保留真实 DDS/MQTT 总线驱动 |
| R-03 | 禁止手改生成物 | ✅ | 0 触碰生成文件 |
| R-06 | 禁止削弱测试或门禁 | ✅ | 135/135 CTest + 10/10 Omnis 全绿 |
| R-07 | 禁止 CI 红时合并 | ✅ | 全量测试 100% 绿灯 |
| R-08 | 禁止修改 edge_core 白名单外文件 | ✅ | edge_core CMakeLists 移除已迁移源码 |
| R-09 | 单个提交对应单个 Ticket | ✅ | 仅包含 AEP-203 总线迁移与驱动验证 |
| P2-02 | 搬文件必须验符号，nm -g 差异为 0 | ✅ | datahub_dds_* 与 datahub_mqtt_* 符号全部无损导出 |
| P2-04 | 保留 Vendored Fallback | ✅ | third-party/CMakeLists.txt 保留源码回退机制 |
