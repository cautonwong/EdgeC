# DCU-D3-06 注释审计

范围：`4233a7c0` 到分支的 15 个改动文件与两处未提交的文档修改。

## 删除

1. `cmd/dcu/dcu_698_param.c` 内部 `dcu_param_t` 的类型说明块：复述 `g_params` 已展示的 13 行形状，且包含变更理由。
2. `cmd/dcu/dcu_698_param.c` 的 `count` 字段注释：注释说“结构字段数”，代码只把它用于 octet 长度。
3. `tests/test_dcu_698_param.c` 的“方向 2、经度 181…”一列：逐条复述下面五个 `bad[]` 断言。
4. `tests/test_dcu_698_param.c` 的“复位后从同一连接重载…”：复述随后三条语句。

## 根因修正

审计的 MUST KILL：`dcu_param_t.count` 同时承诺“octet 最大长度”和“结构字段数”，实际只有 octet 路径读取。字段改名为 `octet_max`，并删掉 `P_UNSIGNED5`/`P_DLU2`/`P_DLU3` 行上的 `.count` 初始化（结构 arity 由 `kind` 决定）。

## 保留

`dcu_698_param.c` 的文件头 `@file/@brief` 按本仓库惯例保留；其余协议条款、目录类上限、ACL 1024 字节约束、4000 RAM 影子与 M127 依赖说明属代码看不出的产品/协议规则。

## 行为核对

删除与改名后重新构建、聚焦测试、全量 ctest、ASAN 与验收均通过，无行为变化。
