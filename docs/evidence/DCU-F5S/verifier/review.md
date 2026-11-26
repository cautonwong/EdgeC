# F5S 对抗评审记录（2026-11-26）

范围：dcu_698_var.c/.h、dcu_event_monitor.c 的 A6 快照身份路径。

## 结论

NO FINDINGS。

## 运行证据

- gdb 断点 `dcu_698_var_encode_axdr_device`：两个事件测试中 `dev=0` 两次（发生+恢复）与 `dev=3` 两次；断点 `dcu_698_var_encode_axdr` 在事件测试零命中（唯一生产调用点在北向 `dcu_698_axdr.c` resolver）。
- 类型链 uint32：`read_shadow`/`datahub_read_shadow_device`/`dcu_tag_map_fn` 均 uint32，无截断。
- 原型仅 `dcu_698_var.h` 一处声明、`dcu_698_var.c` 一处定义。
- 既有二进制：test_dcu_advanced_modules 2/2、test_dcu_698_axdr 44/44、test_dcu_698_var 8/8、test_dcu_router 3/3。

## 未覆盖

- 未重建、未跑全量 ctest/ASAN（评审约定只读）。
- 发现一条既有身份缺口：恢复记录 `start_time` 取自每 OI 一份的 `evt->last_start_time`（`dcu_event_monitor.c:231/247`），已在计划 §12.5 登记。
