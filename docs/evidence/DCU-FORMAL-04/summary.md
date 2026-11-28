# DCU-FORMAL-04 GET-NormalList 编码 CBMC 证明

基线 `bc777aaa`，分支 `dcu/FORMAL-04-get-list-proof`。规格见同目录 `spec.md`。

## 范围

- 被测：`foundation/src/protocol/dlt698_codec.c:dlt698_encode_get_response_list` 及其直接调用的
  `_frame_begin` / `_frame_seal` / `_fits` 路径。
- 未改生产代码语义与帧字节；本单元只新增证明工件。
- `formal/cbmc/dlt698_encode_get_list.c` 直接 `#include` 生产 codec 与 `net_buf.c`，
  `NDEBUG` 模拟 release：`net_buf_simple_add` 的 assert 不是防线，`_fits` 才是。

## 有界模型

- 输出缓冲固定 `FORMAL_MAX_LEN=24` 字节，容量 `cap` 符号化（`cap <= 24`）；
  `__buf/data` 尾对齐到 `24 - cap`，故 `tailroom == cap`，越过 `cap` 即越过存储对象。
- `unwind = 25 = max_len + 1`，`--unwinding-assertions`；无 VLA、无无界循环。
- 符号量：`cap`、地址长度 `sa.len ∈ [1,16]`、条目数 `n`、每条 `dar` 与 `dlen ∈ [0,8]`。
  负载 / `oad` / 地址字节取具体值：被测函数只按长度与计数分支，字节值不影响控制流，
  这样避免 FCS 表查表的符号开销。

## 证明的性质

1. 内存安全：CBMC `--bounds-check/--pointer-check/--signed-overflow-check/--conversion-check`
   覆盖整条写入路径；另断言 `encoded length within cap`、`writes within storage`。
2. 返回值契约：`rc ∈ {0,-1}`；`rc == -1` 当且仅当条目超限（`n > DLT698_GET_LIST_MAX`）
   或容量不足（`need + frame_overhead > tailroom`）。main 的条目 data 恒非空，
   NULL 指针 / 缺失 data 由 `get_list_reject` 具体入口覆盖。
3. vacuity：`__CPROVER_cover(rc == 0)`，成功编码路径可达。

## 实测结果（本机 CBMC 5.12，x86_64）

| mode | 结果 | 耗时 |
| --- | --- | --- |
| proof | `** 0 of 3383 failed`，VERIFICATION SUCCESSFUL | 35 s |
| cover | `[main.coverage.1] ... 'rc == 0': SATISFIED` | 32 s |
| function | `get_list_reject` 6 条断言 SUCCESS | 1 s |
| negative（干净树） | 脚本 rc=0；内部变异副本 CBMC FAILURE 13 处 | 47 s |
| negative（变异树） | 脚本 rc=1（`guard occurrence count is 0`） | <1 s |

命令（manifest runner 的等价行）：

```bash
INC=(-I foundation/include -I .local/linux-x86_64/include -I .local/linux-aarch64/include)
# proof
cbmc formal/cbmc/dlt698_encode_get_list.c "${INC[@]}" \
  --bounds-check --pointer-check --div-by-zero-check --signed-overflow-check \
  --conversion-check -DFORMAL_MAX_LEN=24 --unwind 25 --unwinding-assertions
# cover
cbmc formal/cbmc/dlt698_encode_get_list.c "${INC[@]}" \
  -DFORMAL_MAX_LEN=24 --cover cover --unwind 25
# function
cbmc formal/cbmc/dlt698_encode_get_list.c "${INC[@]}" \
  --bounds-check --pointer-check --div-by-zero-check --signed-overflow-check \
  --conversion-check -DFORMAL_MAX_LEN=24 --function get_list_reject
# negative（由 check_formal_negative.sh 以环境变量调用）
ROOT_DIR=. FORMAL_OUT_DIR=build/formal/dcu CBMC_BIN=cbmc \
  ARTIFACT=formal/cbmc/dlt698_encode_get_list.c \
  bash formal/cbmc/negative/dlt698_encode_get_list_mutation.sh
```

## 负向变异

- 临时副本删除 `    if (!_fits(b, sa, need, 0u)) return -1;` 后 CBMC 报 13 处 FAILURE，
  包括 `[main.assertion.4] dlt698_get_list: writes within storage: FAILURE`、
  `[main.assertion.2] ... -1 iff ...: FAILURE` 与 `_frame_seal_offsets.pointer_dereference.*`。
- `ci/mutation_guards.tsv` 新增 `DCU-FORMAL-04-get-list-fits-guard`：同一 find/replace，
  `build_target=edge_codec`，`test_cmd` 调上面的负向脚本。
  已验证红/绿：变异树 test rc≠0，还原后干净树 test rc=0；`git status` 复原干净。
- `python3 ci/scripts/run_mutation_guards.py --selftest` 通过。

## 设计选择与歧义

- CBMC 5.12 的陷阱：`__CPROVER_assume` 直接作用在 struct 字段上时，后续读取互不约束。
  最小复现里 `sa.len <= 2`、`sa.len == 16`、`sa.len >= 8` 同时 SUCCESS。
  harness 改为先约束局部变量再赋给字段，否则证明域被悄悄放宽。
- `max_len=24` 至少容纳一个单项成功帧（`sa.len=1`、`n=1` DAR 恰为 24 字节）。
  多于一项的成功帧需要更大的界，未纳入本单元；容量不足分支仍按符号 `n` 证明。
- `function` 模式在 runner 中不带 `--unwind`，若进入 FCS 的 `while (len--)` 不会收敛；
  因此 `get_list_reject` 只覆盖在封帧前返回 -1 的路径。成功编码由 main 的 cover 与断言覆盖。
- 负向脚本内部 `timeout 300`：变异源使求解变慢（实测 47–90 s，随机器负载波动）。
- 假设 `sa.len ∈ [1,16]`（与解码侧一致）：函数未校验 `sa.len==0`，该输入会产生畸形帧
  （地址长度字节下溢为 255），但不越界；不属于内存安全缺陷，本单元不改其语义，记为此处歧义。

## 未纳入 / 非目标

- 不把 `dlt698_decode_data` 从递归改为迭代（另立重构单）。
- 不修改 `dlt698_encode_get_response_list` 的公开语义或帧字节。
