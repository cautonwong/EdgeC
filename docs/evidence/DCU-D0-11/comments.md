# DCU-D0-11 注释审计

范围：`78fd6083` 到工作树的 diff，新增的并发脚本、`tests/CMakeLists.txt` 与四个测试文件。

## 结果

删除 0 处。

## 保留

- `tests/CMakeLists.txt` 的并发守卫说明：注册块本身看不出 `/tmp` 状态路径会碰撞。
- `ci/scripts/run_concurrent_binary.sh` 的脚本用途说明：脚本只负责启动两个进程，路径约束在被测 binary 里，脚本本身无法表达。

## 行为核对

四个测试文件的 `/tmp` 路径都改为 `<tag>_<pid>`，helper 与各用例的 unlink 未变；没有注释因这次改动变旧。
