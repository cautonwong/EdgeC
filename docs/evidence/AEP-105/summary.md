# AEP-105 证据包总结 (Summary)

- **工单 ID**: AEP-105
- **标题**: 路径与符号回归门禁集成
- **状态**: Done
- **完成时间**: 2026-08-16

---

## 1. 变更内容与改动说明

1. 编写 `scripts/check_no_old_src.sh`，严防代码库中再次引入对旧 `src/` 源码树的任何物理文件或 CMake 引用。
2. 编写 `scripts/check_symbols_baseline.sh`，自动化断言 `libedge_io.a` 关键全局符号完整性（northbound_mgr, io_executor 等）。
3. 将架构反熵门禁集成至 `.github/workflows/ci.yml`，任何违规改动将在 CI 中自动阻断。

---

## 2. DoD 逐项验收勾选

- [x] `scripts/check_no_old_src.sh` 脚本生效并加入 CI
- [x] `scripts/check_symbols_baseline.sh` 脚本生效并加入 CI
- [x] 故意引入违规或缺失符号可触发门禁红灯阻断
- [x] 无 R-01~R-14 红线违规
