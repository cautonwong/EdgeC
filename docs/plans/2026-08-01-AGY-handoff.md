# AGY 开工指令（用户手动粘贴版）

> 2026-08-01 · 由 Codex 起草，用户转交 AGY 执行；执行结果由 Codex 审核。

AGY 你好：请依据 `/workspaces/edgeos/EdgeC/docs/plans/2026-08-01-ticket-roadmap.md` 实施 EdgeOS/EdgeC 的 41 个 Ticket（Phase 0-6），工作目录为 `/workspaces/edgeos/EdgeC`；其中 T-001（签名私钥历史清理）暂缓执行、等用户决策，T-507（Zephyr 适配）不在本期范围。你必须遵守 EdgeOS 工程宪法：四平面隔离、Zero-Mock（禁止用硬编码或模拟值冒充物理数据，所有入库数据必须可溯源到物理通道/Matrix Actor/契约计算）、Consumer-Driven 接口、非侵入式集成（禁止修改 `modules/hal/` 与 `rtos/` 原厂代码）、禁止硬编码 TagID（一律使用契约编译器生成物）。每个 Ticket 在独立分支 `ticket/T-XXX` 上实施，完成后必须自测：`./build.sh`、`./test.sh`（127 项 CTest）、`cd go && go test ./...`，涉及 Matrix 的 Ticket 再运行 `./test.sh --omnis`；提交时附上 DoD 逐项勾选、测试输出和变更摘要。若用三个 agent 并行，请按该计划 §1.1 分工：Agent-A 负责 T-002~T-006 + T-301~T-305，Agent-B 负责 T-101~T-107 + T-201~T-206，Agent-C 负责 T-401~T-407 + T-501~T-506 + T-601~T-604，共享文件冲突由 Agent-C 协调。全部工作完成后由 Codex 按计划 §4 协议逐 Ticket 审核（完成度/代码质量/测试充分性/回归风险），审核不过不得合并；遇到需求不明确或阻塞时立即停下报告，不要擅自扩大范围、不要用假数据绕过测试、不要合并到 main。
