# DCU-P0 基线记录

跑基线的时间：2026-11-21，提交 `15a51c80b2dd1f6ff94c3ce90bed35ca0027d578`，分支 `dcu/D0-04-baseline`，工作树 `/tmp/dcu-baseline-worktree`。

本任务只记录，不改产品代码。下面每条都是实际执行过的命令与真实退出码。

## 结果

| 项 | 命令 | 结果 |
| --- | --- | --- |
| Release 构建 | `cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release -DCOVERAGE=ON` + `cmake --build . -j$(nproc)` | 通过，1094 个目标，0 编译错误 |
| DCU 单测 | `ctest -R dcu --output-on-failure` | 22/22 通过，退出 0 |
| 698 单测 | `ctest -R dlt698 --output-on-failure` | 6/6 通过，退出 0 |
| 全量单测 | `ctest --output-on-failure -E '^app_platform_e2e$'` | 330/330 通过，185.03 s，退出 0 |
| 特权 E2E | `ctest -R '^app_platform_e2e$'`（本机以 root 直接跑） | 1/1 通过，2.97 s，退出 0 |
| ASAN 构建 | `-DENABLE_ASAN=ON` + 构建 | 通过，退出 0 |
| ASAN DCU 单测 | `ctest -R dcu` | 22/22 通过，退出 0 |
| ASAN 698 单测 | `ctest -R dlt698` | **5/6，1 条失败，退出 8** |
| UBSAN 构建 | `-DENABLE_UBSAN=ON` + 构建 | 通过，退出 0 |
| UBSAN DCU 单测 | `ctest -R dcu` | 22/22 通过，退出 0 |
| UBSAN 698 单测 | `ctest -R dlt698` | 6/6 通过，退出 0 |
| 覆盖率 | `lcov` 采集 + `ci/scripts/check_coverage.py` | 通过（按 CI 顺序，含 E2E 后再采集） |
| 验收 | `TESTPLATFORM=/workspaces/test-platform ./test.sh --omnis dcu` | 2 个任务、6 条用例全部合格，退出 0 |

## 环境

- gcc 11.4.0、cmake 3.22.1、ninja 1.10.1、lcov 1.14、python 3.10.12，8 核。
- WSL2 内核 5.15.153.1；当前用户 uid 0，镜像里没有 `sudo` 命令。
- 工作树从 `15a51c80` 新建 checkout，`build/` 为空开始，未复用其他工作树。

## 失败：ASAN 报栈使用越界

`build-asan/tests/test_dlt698_conformance_suite` 在 `test_get_request_normal` 报：

```
ERROR: AddressSanitizer: stack-use-after-scope
READ of size 1 ... in test_get_request_normal
Address ... is located in stack of thread T0 at offset 524 in frame
    #0 ... in test_get_request_normal
  [512, 1024) 'storage' (line 145) <== Memory access at offset 524 is inside this variable
```

原因在测试辅助函数本身：`tests/test_dlt698_conformance_suite.c:143-150` 的 `decode_frame` 把 `net_buf_simple` 指到自己的局部数组 `uint8_t storage[512]`，函数返回后调用方在 `:217` 读取 `payload`，该指针指向已退栈的帧。生产代码不经过这条路径。

单独运行该测试二进制可稳定复现（退出 1，同一份报告）。Release 与 UBSAN 构建下不触发，因为只有 ASAN 检查作用域生命周期。

本任务不改代码，只记录。修复方向：让调用方提供缓冲，或把 `storage` 改成 `static`（前者更干净）。

## 跳过

`test_dlt698_conformance_suite` 21 条中 20 条通过、1 条跳过：

```
[  SKIPPED ] test_get_record_and_rsd_not_implemented
```

这是已登记的未实现项（GET-Request-Record / RSD 编码），属 D1-15 范围，不是本次环境问题。

## 覆盖率

两次采集，差别只在 E2E 是否已跑过：

| scope | 未跑 E2E | 跑过 E2E（CI 顺序） | floor | 结论 |
| --- | --- | --- | --- | --- |
| ALL | 77.1% | 77.2% | 74.5% | PASS |
| adapters | 74.5% | 74.4% | 66.5% | PASS |
| cmd | **75.9%** | **78.0%** | 76.5% | 未跑 E2E 时 FAIL |
| foundation | 96.5% | 96.5% | 87.0% | PASS |
| modules/edge_core | 75.6% | 76.3% | 75.0% | PASS |
| modules/edge_io | 62.1% | 62.0% | 60.0% | PASS |
| modules/edge_mgmt | 87.3% | 87.3% | 87.0% | PASS |
| modules/edge_stream | 69.3% | 66.2% | 65.0% | PASS |

`cmd` 的 2.1 个百分点来自 `app_platform_e2e` 跑过的产品进程；CI 在采集前会跑这条特权用例，所以 CI 口径是通过。只跑单测就采集会误判为红，这一点以后复现基线时要注意顺序。

## 执行细节

- 构建用 `-j8`，`nproc` 为 8，等价于任务里的 `-j$(nproc)`。
- 覆盖率命令与 CI 相同：`lcov --capture --directory . --output-file coverage.info`、`lcov --remove ... '/usr/*' '*/tests/*' '*/third-party/*'`、`python3 ci/scripts/check_coverage.py --info coverage_clean.info --root <repo>`。
- 验收：CI 会把 test-platform 临时 clone 到 `$RUNNER_TEMP`；本机网络拉包停在 5 MB 不动，改用本机已有的 `/workspaces/test-platform`，其 HEAD 为 `faad172bdce0006ab0ebffd253418b7bdfb35683`，与 `acceptance/testplatform.sha` 锁定值一致。两个验收任务 `dcu_acceptance`、`dcu_northbound` 均退出 0。
- 特权 E2E 在 CI 用 `sudo -E env ...`；本机没有 `sudo`，已以 root 直接执行同一 `ctest` 命令。
- `python3` 依赖只有平台虚拟环境 `.venv` 里齐全（pytest、pyyaml、paho-mqtt），验收走的是平台自带的解释器。

## 没做的

- 没在 GitHub Actions 上重跑这轮 CI；以上都是本机同命令结果。
- 没跑 TSAN、arm64、trimmed 三个 CI 矩阵项——任务只要求 ASAN/UBSAN。

完整输出见 `tests.log`，运行时报告见 `runtime.log`。
