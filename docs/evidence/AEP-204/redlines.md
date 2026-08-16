# AEP-204 红线自查表 (Redlines Check)

| 红线编号 | 红线要求 | 判定 | 说明 |
|---|---|:---:|---|
| R-01 | 未冻结基线不得启动 Phase 1+ | ✅ | Phase 0+1 已闭环 |
| R-02 | 物理真实性，禁止伪造数据 | ✅ | 真实 nm -u 检查与白名单门禁 |
| R-03 | 禁止手改生成物 | ✅ | 0 触碰生成文件 |
| R-06 | 禁止削弱测试或门禁 | ✅ | 136/136 CTest -j1 + 10/10 Omnis 全绿，新增 check_core_symbols.sh 严苛门禁 |
| R-07 | 禁止 CI 红时合并 | ✅ | 全量测试 100% 绿灯 |
| R-08 | 禁止修改 edge_core 白名单外文件 | ✅ | 白名单已正式落盘 contracts/schema/core_readonly_whitelist.txt 并经门禁验证通过 |
| R-09 | 单个提交对应单个 Ticket | ✅ | 仅包含 AEP-204 核心符号收窄、门禁加固与单测补齐 |
| P2-05 | 依赖单向流动 | ✅ | nm -u 验证 libedge_core.a 对第三方引擎 0 未解析依赖 |
