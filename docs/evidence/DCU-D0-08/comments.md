# DCU-D0-08 注释审查

工具：`pstack:no-comments`（comment-sicko 子代理，opus），范围 `tests/test_dlt698_conformance_suite.c` 全文件注释，改动面只有 `decode_frame` 一处。

## 接受的处理

1. 删除 1 行注释（原第 145 行）：
   `/* payload 指向调用方的帧缓冲：它必须比返回的指针活得久。 */`
   理由：它描述的是文件内私有辅助函数的行为，不是公开 API 契约，也不是依赖/平台/协议规则。审查人核对后确认说法属实，但属实不构成保留条件。

2. 接受 `MUST KILL`：把辅助函数改名为 `decode_frame_borrows`。这是被删注释的可执行编码——名字本身表明 `payload` 借用调用方缓冲、不拥有它。11 个调用点一并改名；断言、期望字节、用例集合不变。

## 未接受的

没有。报告里没有越界编辑；未改产品代码，未动 DAR 断言。

## 保留的注释

文件头与各测试函数的 DL/T 698.45 条款页码、HCS/FCS 覆盖范围、附录 D/H 的已知答案来源、H.3.1/H.3.2/H.4.1/H.5.1 向量出处、DAR 结构（表 34 / 6.3.3.11）、缺失编码器的 D1 追踪号，全部保留。审查人逐条列出，均为协议条款或错误码约束，不是对代码行为的复述。

## HOLD

无。

## 审查人报告的越界观察（未修，记录）

- `(void *)frame` 去掉 `const`：`net_buf_simple_init_with_data()` 收 `void *`，而解码器只读。要消掉强制转换得改 foundation 的共享 API，超出本次缺陷修复范围。
- `b.len = len` 把 `size_t` 收窄到 `uint16_t`：`net_buf_simple_init_with_data()` 本身也把 `size` 收成 `uint16_t`。现有测试向量只有几十字节，不会触发；超过 65535 字节的帧才会截断。

两条都记在这里，不在本任务里改。

## 审查后的验证

- `git diff --check` 通过。
- ASAN 构建 + `ctest -R dlt698`：6/6，退出 0。
- 全量 ASAN `ctest -E '^app_platform_e2e$'`：330/330，退出 0。
- Release `ctest -R 'dcu|dlt698'`：28/28，退出 0。
