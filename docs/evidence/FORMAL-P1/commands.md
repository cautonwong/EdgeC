# FORMAL-P1 命令

工作目录：`/tmp/iems-fix/formal-p1-wt`（`worktree-formal-p1`，基线 `1f7f2133`）。

```bash
# 工具
source /opt/formal/env.sh
java -version
sha256sum /opt/formal/tla2tools.jar        # 7e53aa63...d0fb84f
java -cp /opt/formal/tla2tools.jar tlc2.TLC -h | head -5   # Version 2026.11.15.002146

# TLC 正例：在仓库外的副本上跑，避免 TLC 在仓库内写 states/ 和 TTrace 文件
mkdir -p /tmp/tlc-p1 && cp formal/tla/Arbiter.tla formal/tla/Arbiter.cfg /tmp/tlc-p1/
cd /tmp/tlc-p1 && timeout 120 java -XX:+UseParallelGC -cp /opt/formal/tla2tools.jar \
  tlc2.TLC -cleanup -workers auto -config Arbiter.cfg Arbiter.tla

# 负例：仓库 cfg，只把 RecoveryGate 翻成 FALSE
mkdir -p /tmp/tlc-p1-neg
cp formal/tla/Arbiter.tla formal/tla/negative/ArbiterNoRecovery.cfg /tmp/tlc-p1-neg/
cd /tmp/tlc-p1-neg && timeout 60 java -XX:+UseParallelGC -cp /opt/formal/tla2tools.jar \
  tlc2.TLC -cleanup -workers auto -config ArbiterNoRecovery.cfg Arbiter.tla
# rc=12, Error: Invariant NoDispatchInRecovery is violated.

# 串包临时 cfg：在 mktemp 里只删掉 NoDispatchInRecovery 这一行，不复制 .tla
grep -vx 'INVARIANT NoDispatchInRecovery' ArbiterNoRecovery.cfg > crosstalk.cfg
timeout 60 java -XX:+UseParallelGC -cp /opt/formal/tla2tools.jar \
  tlc2.TLC -cleanup -workers auto -config crosstalk.cfg Arbiter.tla
# rc=12, Error: Invariant LateResponseNotAssignedToNext is violated.

# 门禁（在仓库根）
./ci/scripts/check_formal.sh --strict
./ci/scripts/check_formal_negative.sh --strict
python3 ci/scripts/check_formal_sync.py --root .
python3 ci/scripts/check_formal_isolation.py --root .

# 回归
python3 ci/scripts/run_guard_selftest.py
cd build && ctest --output-on-failure
cd .. && ./ci/scripts/run_arch_gates.sh build 1f7f2133
```

`check_formal.sh` 把 spec/cfg 复制到 `build/formal/tla/` 后运行，仓库工作树不出现 TLC 的 `states/` 或 `Arbiter_TTrace_*.tla`。负例的临时 cfg 与 spec 副本都在 `mktemp -d` 下。
