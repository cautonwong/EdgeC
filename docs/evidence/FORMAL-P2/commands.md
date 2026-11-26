# FORMAL-P2 命令

工作目录：`/tmp/iems-formal/wt`（`worktree-formal`，基线 `42e1360d`）。

```bash
# 构建（ARCH-GRAPH 强模式与 build freshness 需要 compile_commands 与完整 archive）
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build -j"$(nproc)"
ninja -C build -n        # 必须输出 no work to do

# 形式化门禁（--strict：缺工具失败；版本/哈希不符无条件失败）
./ci/scripts/check_formal.sh --strict
./ci/scripts/check_formal_negative.sh --strict
python3 ci/scripts/check_formal_sync.py --root .
python3 ci/scripts/check_formal_isolation.py --root .
python3 ci/scripts/test_formal_manifests.py

# 工具链
java -version                                              # openjdk 17
sha256sum formal/tools/tla2tools.jar                       # 7e53aa63...d0fb84f
env -u TLA2TOOLS ./ci/scripts/check_formal.sh --strict     # 没有 /opt/formal/env.sh 时回退 vendored jar

# 分片证明（临时；跑完删除，git status 必须干净）
#   formal/cbmc/dcu_probe.c + formal/manifest/dcu.tsv（1 property）
./ci/scripts/check_formal.sh --strict
cat build/formal/dcu/run-manifest.tsv

# 红例
#   未登记：echo 'int x;' > formal/cbmc/stray.c && git add ... && check_formal_sync.py --root .
#   缺 manifest：check_formal_sync.py --root <empty-temp-dir>
#   计数不符：临时把 proof 行 property_count 1012 改成 1011 后 check_formal.sh --strict

# 全量架构/反熵门禁（含 formal、ARCH-GRAPH 强模式、selftest）
bash ci/scripts/run_arch_gates.sh build 6ad00780
python3 ci/scripts/check_ci_gate_parity.py --root .
python3 ci/scripts/run_guard_selftest.py
```

分层日志：`build/formal/<owner>/run-manifest.tsv`、`build/formal/<owner>/<artifact>.<mode>.log`、`build/formal/<owner>/tla/`、`build/formal/<owner>/negative/`。CI 上传整个 `build/formal`。

原始捕获：`docs/evidence/FORMAL-P2/*.txt`（strict 正负例、sync/isolation/parity、selftest、manifest 反例、分片证明、红例、toolchain、vendored-fallback、arch-gates-final、tip）。
