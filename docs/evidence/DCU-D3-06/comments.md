# DCU-D3-06 注释审计

两轮：`4233a7c0` 基线的首次审计，以及时钟端口返工后对 `git diff 2f502b50` 的复审计（`pstack:comment-sicko`）。范围含 `.audit/` 之外的全部改动文件与新增 `cmd/dcu/dcu_clock_port.{c,h}`。

## 第一轮删除（已随首个提交落地）

1. `cmd/dcu/dcu_698_param.c` 内部 `dcu_param_t` 的类型说明块：复述 `g_params` 已展示的 13 行形状，且包含变更理由。
2. `cmd/dcu/dcu_698_param.c` 的 `count` 字段注释：注释说“结构字段数”，代码只把它用于 octet 长度。
3. `tests/test_dcu_698_param.c` 的“方向 2、经度 181…”一列：逐条复述下面五个 `bad[]` 断言。
4. `tests/test_dcu_698_param.c` 的“复位后从同一连接重载…”：复述随后三条语句。

## 第二轮删除（时钟端口返工）

- 失实/过时：`dcu_698_appconn.c` 的“4000 的 A2/A3/A4 只读（时钟服务未接入）”（ACL 权限是 3，端口已注入）；验收文件头的“Class 8 参数 RAM 影子”。
- 复述代码：`dcu_storage.c` 的 `-1` 哨兵注释（改为 `bool has_old` + `old_len`）；`dcu_node.c` 的组合根两行说明；`tests/test_dcu_698_param.c` 的 7 处（无落盘行、DAR 7、DAR 2、不部分写入、绕过保存端口、A3/A4 落盘、不进 `dcu_params`、重开 DB）；`tests/test_dcu_698_axdr.c` 的 2 处（mask 顺序、月 13）；`tests/test_dcu_698_object_model.c` 的 2 处（白名单重复说明、30/29 算术）；`tests/test_driver_dlt698.c` 的 3 处（29 对象、只读 DAR 3、31 对象拒绝）；`tests/test_dcu_698_appconn.c` 的 1 处（4000 属性 3/4）；验收用例的 docstring 与 BCD 内联注释。

## 第二轮根因修正

`_default_rc` 不再硬编码 `attr == 1`，改用 `dcu_698_class_defines_attr(IC_PARAM, attr)`，类上限表成为唯一来源，注释随之删除。

## 拒绝的 MUST KILL

- 删除 `dcu_north_accept_axdr` 的类 8 前置拦截、把 4000 的 A3/A4 塞进对象 mask：这是已评审的目录上限设计（Class 8 类只定义属性 1/2），改动属于功能/结构变更，不在注释审计范围。保留 `dcu_698_object_model.c` 中一行说明“对象行只广告属性 2；4000 的 A3/A4 由参数模块接管”，避免下一个贡献者把差异当缺陷修掉。
- `dcu_node.c` 的“4000 A2 走时钟端口，其余参数从这里落盘”：解释为什么保存端口收不到 4000 A2，属看不到的缺省行为，保留。

## 保留

`dcu_698_param.c` 的文件头 `@file/@brief` 按本仓库惯例保留；`dcu_698_param.h`、`dcu_clock_port.h`、`dcu_storage.h` 的 Doxygen 属公开 API 契约；`driver_dlt698.c` 的 `DRIVER_PRIV_SIZE` 预算说明有 `_Static_assert` 与 1024 上限佐证。

## 行为核对

删除与 `_default_rc` 改写后重新跑：干净构建 + warning gate 0、全量 ctest 341/341、ASAN 5/5、守卫自测、fanalyzer 0 新报告、验收 2/2（northbound 20/20）。
