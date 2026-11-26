# Comment Sicko: 本切片新增注释均为代码不能表达的非显然 why（poller 时标来源与 0=未发生、60xx DAR 2 的原因、task_mask 语义沿用）。无叙述性注释。

## D2-08b

新增注释均为非显然 why：submit 任务掩码语义、note_tx/rx 的计数口径（含重试/超时）、twin/demo 不计数、visited/成功数只计每表首次访问、absent 时标的 FFFF 约定。无叙述性注释。

## D2-08b 评审修复

新增注释均为非显然 why：完成判据需等待全部 pending 项、重开时作废旧 visit 标记、提交失败不计发送。
