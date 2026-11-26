# FORMAL-P0 命令

工作目录：`/tmp/iems-fix/formal-p0-wt`（`worktree-formal-p0`，基线 `2bbf729e`）。

```bash
# 工具
source /opt/formal/env.sh
cbmc --version                      # 5.12 (cbmc-5.12)
sha256sum "$(command -v cbmc)"      # ef72f838...ee0d

# 主门禁
./ci/scripts/check_formal.sh --strict

# 负例（只改 mktemp 副本）
./ci/scripts/check_formal_negative.sh --strict

# manifest 与隔离
python3 ci/scripts/check_formal_sync.py --root .
python3 ci/scripts/check_formal_isolation.py --root .

# 正反夹具
python3 ci/scripts/run_guard_selftest.py

# CI 与本地 runner 一致
python3 ci/scripts/check_ci_gate_parity.py --root .

# unwind 三档测量（主入口）
cbmc formal/cbmc/modbus_decode.c -Ifoundation/include -I.local/linux-x86_64/include \
  -DFORMAL_MAX_LEN=16 --bounds-check --pointer-check --div-by-zero-check \
  --signed-overflow-check --conversion-check --unwind 17 --unwinding-assertions
cbmc formal/cbmc/modbus_decode.c -Ifoundation/include -I.local/linux-x86_64/include \
  -DFORMAL_MAX_LEN=64 --bounds-check --pointer-check --div-by-zero-check \
  --signed-overflow-check --conversion-check --unwind 65 --unwinding-assertions
cbmc formal/cbmc/modbus_decode.c -Ifoundation/include -I.local/linux-x86_64/include \
  --bounds-check --pointer-check --div-by-zero-check --signed-overflow-check \
  --conversion-check --unwind 261 --unwinding-assertions

# CRC 拒绝具体入口
cbmc formal/cbmc/modbus_decode.c -Ifoundation/include -I.local/linux-x86_64/include \
  -DFORMAL_MAX_LEN=16 --bounds-check --pointer-check --div-by-zero-check \
  --signed-overflow-check --conversion-check --function crc_reject

# vacuity cover
cbmc formal/cbmc/modbus_decode.c -Ifoundation/include -I.local/linux-x86_64/include \
  -DFORMAL_MAX_LEN=16 --cover cover --unwind 17

# 全量
cd build && ctest --output-on-failure
cd .. && ./ci/scripts/run_arch_gates.sh build 2bbf729e
```
