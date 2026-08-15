# AEP-102 红线自查表 (Redlines Check)

| 红线编号 | 红线要求 | 判定 | 说明 |
|---|---|:---:|---|
| R-01 | 未冻结基线不得启动 Phase 1+ | ✅ | Phase 0 已通过 |
| R-03 | 禁止手改生成物 | ✅ | 0 触碰生成代码 |
| R-06 | 禁止削弱测试或门禁 | ✅ | 134/134 CTest 保持全绿 |
| R-08 | 禁止修改 edge_core 白名单外文件 | ✅ | 仅改动 edge_io/CMakeLists.txt |
| R-09 | 单个提交对应单个 Ticket | ✅ | 仅包含 AEP-102 CMake 重构 |
