# DCU-D2-05a-r：/no-comments（Comment Sicko）

范围：`git diff d42ce250` + 工作树，4 个文件（两个新源文件、内部头、CMake）。

## 接受删除（2 处）

- `dcu_698_axdr_internal.h` 的文件用途横幅（"两个 codec 文件共用的 A-XDR 游标与原语"）：grep 两次 include 即可看到消费者。
- `dcu_698_axdr_take_ms` 声明上方的"MS 共用、实现留在 axdr.c"说明：原型与唯一实现本身可见。

## 保留

- `_time_tag_absent` 的 TimeTag 约束注释：标准 6.1.1.5 的时效判定未实现，存在 TimeTag 时按 DAR 7 拒绝而不是静默忽略，这是代码无法表达的协议约束。
- 头文件 guard 的 `#endif` 标签、6014 的 MS 线格式注释、表49 的时区注释均为既有上下文，未改。

## 其他

- 无 MUST KILL，无 workaround 代码；本轮是纯移动，未改逻辑、断言或门禁。
- 删除仅发生在新增文件的注释上；行为面由 7/7 scheme_mgmt 精确向量、15/15 受影响 ctest、ASAN detect_leaks=1、验收 2/2 覆盖。

## 结论

删除 2 处注释；无代码变化；拆分行为不变。
