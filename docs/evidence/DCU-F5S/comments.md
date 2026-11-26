# DCU-F5S 注释自审

范围：测试提交 `d99574b6`、修复提交 `85cb3a04` 与返工提交 `c8796002` 的新增注释
（未跑 comment-sicko；本单任务未要求，按仓库注释纪律人工自评）。

## 保留

- `cmd/dcu/dcu_698_var.h` 的 `dcu_698_var_encode_axdr_device` 声明注释：说明
  device_id 是稳定 slot 身份、OAD 低字节仍只作属性/元素选择；这是新 API 与旧 API
  的唯一区别，签名本身看不出。
- `tests/test_dcu_advanced_modules.c` 的 `/* F5：A6 关联数据快照必须按事件的 slot_id
  取表。 */`：固定 `snapshot_count` 从 0 恢复为 1 的意图。
- 测试中 `/* 两张表的电压影子：slot 0 是 A，slot 3 是 D... */`：解释为什么发布两份
  影子、为什么断言 raw 1500 而不是只看 len；没有它就看不出测试在区分哪两只表。
- 测试中 `/* D 的 A/B/C 相电压 ... 旧码从 slot 0 的 A 表取到 1000/1010/1020。 */`：
  记录红/绿判据，断言常量本身不说明来源。

## 删除

- 初版在 `dcu_698_var.c` 与 `dcu_event_monitor.c` 各留的“本地原型 + 文件锁”注释：
  返工后原型移入 `dcu_698_var.h`，两份本地声明与解释性注释一并删除，避免第二份契约。

## 行为核对

修复只改快照取表的 device 来源；北向 OAD GET 路径（`dcu_698_var_encode_axdr`）与
`dcu_698_var_read_value` 语义不变。返工只移动声明。`test_dcu_advanced_modules` 2/2、
`ctest -R 'dcu|dlt698'` 45/45 通过。
