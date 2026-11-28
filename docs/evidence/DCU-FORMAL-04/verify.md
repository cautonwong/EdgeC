# DCU-FORMAL-04 独立验证 (`verify.md`)

规格：[spec.md](spec.md)。基线 `bc777aaa`，worker HEAD `1873e661`，合并 `82239c41`。

## 独立重跑（合并树）

| 项 | 命令（manifest 同参） | 结果 |
| --- | --- | --- |
| proof | `cbmc formal/cbmc/dlt698_encode_get_list.c -Ifoundation/include -I.local/linux-x86_64/include --bounds-check --pointer-check --div-by-zero-check --signed-overflow-check --conversion-check -DFORMAL_MAX_LEN=24 --unwind 25 --unwinding-assertions` | rc=0，`0 of 3383 failed`，VERIFICATION SUCCESSFUL |
| cover | 同参数 `--cover cover` | rc=0，`[main.coverage.1] 'rc == 0': SATISFIED` |
| function | 同参数 `--function get_list_reject` | rc=0，0 FAILURE |
| negative | `ROOT_DIR=. FORMAL_OUT_DIR=<tmp> ARTIFACT=... bash formal/cbmc/negative/dlt698_encode_get_list_mutation.sh` | rc=0（脚本内部变异后检出失败才返回 0） |

## 边界复核

`FORMAL_MAX_LEN=24` 下成功路径只能容纳 1 项（一帧 24 字节：6 APDU + 5 OAD/标签 + 1 数据 + 12 帧头地址）。我尝试把界提到 48 覆盖 3 项：CBMC 在 300 秒内未完成（7,727,538 变量、41,221,323 子句，rc=124）。因此本证明的合法域是“拒绝路径对任意 symbolic n，成功路径 n=1”；多条目成功写路径未证明，登记为 §12.5 缺口，不放宽断言。

## 结论

**VERIFIED（有界域）**：证明、cover、函数断言与负向脚本均可重跑且通过。未覆盖：多条目成功编码；扩大界或换有界迭代证明是后续单。
