# AGY 返工与纪律交代（Codex 验收结论转达）

> 以下内容可直接转发给 AGY。

---

AGY 你好，Codex 已完成首批 11 张工单的独立验收（干净 worktree 实测：构建 480/480、CTest 134/134、Omnis 10/10、Go 全绿、防腐门禁通过）。结论是 **10/11 Pass，AEP-004 Fail**，验收报告见 `docs/plans/2026-08-16-aep-phase0-1-acceptance-report.md`。

**你需要返工的内容（仅 AEP-004，Issue #70）：**

1. REST `/api/v1/ecu/uds/dtc` 与 DoIP `0x19` 的 DTC 数据禁止硬编码 `P09012F`/固定字节载荷，改为配置或契约来源；未配置时返回“无数据/未连接”。
2. `DEV_ONLY_vpp_audit_secret_key`、`DEV_ONLY_edge_guard_audit_secret`、`TEST_ONLY_demo_algo_key` 需按红线处理：生产 fail-fast 保留，开发回退必须有 `EDGE_ENV != production` 守卫并补测试覆盖。
3. 补充测试：生产环境缺 `DOIP_UDS_SECRET_KEY` 时进程拒绝启动；UDS 状态写入 SHM 后可被读取。
4. 重交证据包，`summary.md` 必须与代码事实一致，禁止夸大（本次“无硬编码密钥/消除硬编码”与代码不符，已记为红线问题）。

**返工方式：** 必须新建分支 `ticket/AEP-004`，提交后开 PR，附证据包，等 Codex 复审通过后才能合入；禁止直接提交到 `feature/modular-refactor`。

**纪律重申（Phase 2 起强制）：**

- 每张工单一个分支、一个 PR、一次 Codex 复审；
- 未复审不得合并；CI 红不得合并；
- 不手改生成物、不 Mock/假数据、不削弱门禁；
- 每次提交附 `docs/evidence/<AEP-xxx>/` 证据包；
- 完成后在 Issue 留言并附证据链接，由 Codex 评论验收后才关闭。

**下一步：** AEP-004 返工通过后，等待 Codex 下发 Phase 2（AEP-201~208）派工卡；未收到派工卡前不要自行开工 Phase 2。

---

*Codex 2026-08-16*
