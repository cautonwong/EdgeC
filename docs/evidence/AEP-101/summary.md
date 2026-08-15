# AEP-101 证据包总结 (Summary)

- **工单 ID**: AEP-101
- **标题**: 平移 4 个存活文件至 modules/edge_io/src/
- **状态**: Done
- **完成时间**: 2026-08-16

---

## 1. 变更内容与改动说明

1. 将旧 `src/` 中仍在使用的 4 个存活源文件平移至 `modules/edge_io/src/`：
   - `src/main_loop/northbound_mgr.c` $\rightarrow$ `modules/edge_io/src/northbound_mgr.c`
   - `src/main_loop/sched_engine.c` $\rightarrow$ `modules/edge_io/src/sched_engine.c`
   - `src/main_loop/sched_port.c` $\rightarrow$ `modules/edge_io/src/sched_port.c`
   - `src/platform/linux/io_executor_linux.c` $\rightarrow$ `modules/edge_io/src/io_executor_linux.c`
2. 保持对外符号接口 100% 一致性，`nm -g` 比对全局符号完整无丢失。

---

## 2. DoD 逐项验收勾选

- [x] 4 个存活文件平移至 `modules/edge_io/src/`
- [x] 全局搜索 `src/main_loop` 与 `src/platform` 在 `modules/` 目录下命中数为 0
- [x] `nm -g libedge_io.a` 关键符号导出无遗漏
- [x] CTest 134/134 保持 100% 全绿
- [x] 无 R-01~R-14 红线违规
