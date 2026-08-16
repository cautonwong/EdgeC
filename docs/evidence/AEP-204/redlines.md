# AEP-204 红线自查表 (Redlines Check) - Round 2

| 红线编号 | 红线要求 | 判定 | 说明 |
|---|---|:---:|---|
| R-01 | 未冻结基线不得启动 Phase 1+ | ✅ | Phase 0+1 已闭环 |
| R-02 | 物理真实性，禁止伪造数据 | ✅ | 真实 nm -u 检查、真实产品函数调用、真实 EDGE_ENV 守卫测试 |
| R-03 | 禁止手改生成物 | ✅ | 0 触碰生成文件 |
| R-06 | 禁止削弱测试或门禁 | ✅ | 136/136 CTest -j1 + 10/10 Omnis 全绿，门禁接入 ci.yml 且启用 STRICT_CORE_GATE=1 |
| R-07 | 禁止 CI 红时合并 | ✅ | 全量测试 100% 绿灯 |
| R-08 | 禁止修改 edge_core 白名单外文件 | ✅ | 白名单 contracts/schema/core_readonly_whitelist.txt 落盘并经 strict 门禁验证 |
| R-09 | 单个提交对应单个 Ticket | ✅ | 仅包含 AEP-204 整改项 |
| P2-05 | 依赖单向流动 | ✅ | nm -u 验证 libedge_core.a 对底层外部引擎 0 依赖，cyclonedds 已从 edge_core 彻底清除 |
