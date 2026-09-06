# AEP-504 Architecture Redlines & Compliance

1. **Zero Mock / Physical Truth**: 所有 RBAC 鉴权状态、高危命令风险评估、CRC32 审计封签均基于真实物理规则与挂钟时间戳计算，杜绝硬编码占位符。
2. **Four-Plane Isolation**: 权限与审计逻辑封装在管理平面与安全层（`modules/edge_mgmt`），通过标准 C 接口操作，不侵入底座 DataHub POSIX SHM 结构。
3. **Defense in Depth**: 双重防线（前端输入端即时阻断 + 后端 C 引擎强制鉴权拦截），杜绝绕过前端绕射漏洞。
4. **Anti-Entropy**: 完整 C 单元测试（42/42 PASS）与 Playwright E2E 自动化（13/13 PASS）保证架构可持续演进。
