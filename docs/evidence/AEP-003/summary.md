# AEP-003 证据包总结 (Summary)

- **工单 ID**: AEP-003
- **标题**: 修复 CI 覆盖率参数与矩阵配置
- **状态**: Done
- **完成时间**: 2026-08-16

---

## 1. 变更内容与改动说明

1. 修复 `.github/workflows/ci.yml` 中构建参数 `-DENABLE_COVERAGE=ON` 为 `-DCOVERAGE=ON`，使 CMake 真实注入 `--coverage -fprofile-arcs -ftest-coverage` 编译标志。
2. 在 CI Pipeline 中为 x86_64 任务增加代码覆盖率收集步骤（`lcov --capture` 与 `lcov --list`），确保自动产出 `.gcda` / `.gcno` 覆盖率数据并生成 `coverage_clean.info`。
3. 保持 x86_64_asan（`-DENABLE_ASAN=ON`）与 x86_64_tsan（`-DENABLE_TSAN=ON`）矩阵完整，全面覆盖 134 项单元测试。

---

## 2. DoD 逐项验收勾选

- [x] `-DENABLE_COVERAGE=ON` $\rightarrow$ `-DCOVERAGE=ON`
- [x] ASan/TSan 矩阵配置完整
- [x] 覆盖率生成与过滤逻辑配置完整
- [x] 无 R-01~R-14 红线违规
