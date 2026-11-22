# DCU-D0-08 修复 ASAN 栈使用越界

基线提交 `5c808baf`，分支 `dcu/D0-08-asan-frame-lifetime`，工作树 `/tmp/dcu-baseline-worktree`。

## 问题

`x86_64_asan` 矩阵跑全量 `ctest -E '^app_platform_e2e$'` 时，`test_dlt698_conformance_suite` 报 `stack-use-after-scope`，从 D0-05 合入后 main 一直红。

根因在测试辅助函数 `decode_frame`（`tests/test_dlt698_conformance_suite.c:143`）：它把 `net_buf_simple` 指向自己的局部 `uint8_t storage[512]`，返回的 `payload` 指向这个已退栈的帧。调用方在返回后读 `payload`（首处 `:217`），读到的是死内存。生产代码不经过这条路径。

## 改法

- 删掉局部 `storage`，`net_buf_simple` 直接借用调用方的帧字节（`net_buf_simple_init_with_data(&b, (void *)frame, len)`），并显式 `b.len = len`，因为该初始化函数把 `len` 置 0。
- 每个调用点的帧缓冲本来就活得比返回的 `payload` 久：`out->b.data` 在 `net_buf_unref` 之前，局部 `frame[]` 在同一函数作用域内。
- 按注释审查结论，辅助函数改名为 `decode_frame_borrows`，用名字表达借用语义，替代原先那句说明注释。11 个调用点同步改名。
- 没有用 `static storage`：那只是掩盖生命周期缺陷。
- 断言、期望字节、用例集合均未改动；未触碰 DAR 断言。

## 结果

| 项 | 修复前 | 修复后 |
| --- | --- | --- |
| ASAN `ctest -R dlt698` | 5/6，退出 8 | 6/6，退出 0 |
| ASAN 全量 `ctest -E '^app_platform_e2e$'` | 该用例失败 | 330/330，退出 0 |
| Release `ctest -R 'dcu\|dlt698'` | — | 28/28，退出 0 |
| `git diff --check` | — | 退出 0 |

修复后同一个 ASAN 二进制直接运行：20 条通过、1 条跳过（`test_get_record_and_rsd_not_implemented`，已登记的 D1-15 未实现项），无 ASAN 输出，退出 0。

## 守卫

`x86_64_asan` 矩阵作业就是这类缺陷的可执行守卫：它跑全量 ctest，任何栈生命周期越界都会让它变红。本缺陷是测试辅助函数里的一次性生命周期错误，再写一条静态检查没有真实收益，因此不新增守卫脚本。

## 越界观察（未修）

注释审查顺带指出两处，都属 foundation 共享 API，不在本次范围：

- `(void *)frame` 去掉 `const`：`net_buf_simple_init_with_data()` 收 `void *`，解码器只读。
- `b.len = len` 把 `size_t` 收窄到 `uint16_t`；该初始化函数同样把 `size` 收成 `uint16_t`。现有向量只有几十字节，超过 65535 字节的帧才会截断。

## 没做的

- 没在 GitHub Actions 上重跑这一轮；以上为本机同命令结果。
- 没改产品代码，没改 `docs/plans/`。

完整输出见 `tests.log`，ASAN 前后报告见 `runtime.log`，注释审查见 `comments.md`。

## 后续（D0-08 follow-up）

`b.len = len` 的 `size_t` → `uint16_t` 收窄在 `check_build_warnings.py` 下是真实 gate 失败。补上界检查并显式转换：

```c
if (len > UINT16_MAX) return -1;
b.len = (uint16_t)len;
```

借用语义、调用点与向量均不变。`python3 ci/scripts/check_build_warnings.py --log <build.log> --root .` 输出 `No new violations`。
