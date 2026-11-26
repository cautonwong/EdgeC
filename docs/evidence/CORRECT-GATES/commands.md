# CORRECT-GATES 证据

工作目录：`/tmp/iems-fix/correct-wt`（`worktree-correct-gates`，基线 `1f7f2133`）。

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DENABLE_WEB_BUILD=OFF
cmake --build build -j4
```

修复前实测：首次 clean build 后 `ninja -n` 仍报 323 条待执行边，IDL 生成 custom command 在 `1238/1240` 才运行，`datahub_dds.c.o` 等对象早于头文件更新。`tests/CMakeLists.txt` 给 `edge_infra` 与三个直接包含生成头的测试补上 `generate_idls` 依赖后，生成步骤先于消费者完成，一次构建即收敛。

三段输出（stdout+stderr，末尾带 rc）：

- `fresh-output.txt`：构建完成后 `check_build_freshness.py --root . --build build` 通过。
- `stale-output.txt`：`touch modules/edge_mgmt/src/app_manifest.c` 后同一命令失败。
- `recovered-output.txt`：`cmake --build build -j4` 后同一命令再次通过。

```bash
python3 ci/scripts/check_build_freshness.py --root . --build build
touch modules/edge_mgmt/src/app_manifest.c
python3 ci/scripts/check_build_freshness.py --root . --build build
cmake --build build -j4
python3 ci/scripts/check_build_freshness.py --root . --build build
```
