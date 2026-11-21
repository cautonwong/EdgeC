# DCU-D0-10 修复 bus.json 读空文件

基线 `7658967a`（D1-15b 已合入），分支 `dcu/D0-10-atomic-bus-log`。

## 问题

`./test.sh --omnis dcu` 偶发红：`test_dcu_acquisition.py::_bus_requests` 解析到空文件。根因是 `acceptance/dcu/dcu_product.py` 用 `Path.write_text` 直接写 `bus.json`——该调用先截断再写，台架进程还在跑时用例去读，就会看到零字节或半截 JSON（四处写入，行 48/57/194/269）。

## 改动

- `acceptance/dcu/bench.py` 新增 `dump_json_atomic(path, obj)`：同目录 `mkstemp` → 写入 → `flush()` + `os.fsync()` → `os.replace()`。同目录保证 replace 原子；异常时删除临时文件再抛出。
- `dcu_product.py` 四处 `BUS_LOG.write_text(...)` 全部改为 `dump_json_atomic(BUS_LOG, _seen)`；`dcu_exit.json` 的写入不属总线计数，未动。
- 顺手修掉审查发现的台架缺陷：地址无法编码时原来改用 `METER_ADDRESS` 应答，把坏请求伪装成合理响应；现在抛 `RuntimeError`（`py_compile` 通过，正常路径不受影响）。
- 新增 `acceptance/dcu/tests/test_atomic_bus_log.py`：读者线程紧循环读取并解析，主线程连续 500 次原子写不同载荷；任何一次解析失败或文件消失都记为错误，最后断言内容等于最后一个载荷。没有用 sleep 做同步（Event 起停），读者也没有重试来掩盖撕裂。

只改 DCU 台架这两个文件与新增测试；其他产品的同类写法留给各自票。

## 验证

| 项 | 结果 |
| --- | --- |
| `./test.sh --omnis dcu` 第 1 次 | 退出 0：dcu-acceptance 3 passed（含新用例）、dcu-northbound 7 passed |
| 第 2 次 | 退出 0：3 + 7 |
| 第 3 次 | 退出 0：3 + 7 |
| 守卫自检 | 通过 |
| `git diff --check` | 干净 |

第一次与第二次尝试（未计入）失败是另一个原因：`build/products/dcu/bin/dcu` 还是 D1-05 合并前的产物，`60120200` 回 DAR 6。重建 `build/` 后 7/7，随后连续三次绿。这与 D0-10 的改动无关，但记录在 `runtime.log` 里，避免把旧二进制的红当成新用例的证据。

## 没有做的

- 其他产品（iems 等）的 `write_text` 没有改，按派工留给各自的票。
- 没有改 `test_dcu_northbound.py`（dcu-dev 所有）。
- 没在 GitHub Actions 上重跑。
