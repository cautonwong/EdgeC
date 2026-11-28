# DCU-FORMAL-06 独立验证报告

## 1. 独立预期推导
- **标准与契约依据**：`foundation/src/protocol/dlt698_codec.c` 的帧开销 `_frame_overhead(sa) = 11 + sa->len` 与各响应 APDU 固定长度：
  - `dlt698_encode_set_response`：`86 | 01 | PIID(1) | OAD(4) | DAR(1) | Follow(1) | TimeTag(1)` = `10` 字节；最小帧长（`sa.len = 1`）= `12 + 10 = 22` 字节。
  - `dlt698_encode_connect_response`：`82(1) | PIID(1) | vendor(24) | ext(8) | ver(2) | conf(8) | func(16) | 7字节常量 | timeout(4) | result(1) | auth(1) | Follow(1) | TimeTag(1)` = `75` 字节；最小帧长 = `12 + 75 = 87` 字节。
  - `dlt698_encode_release_response`：`83(1) | PIID(1) | result(1) | Follow(1) | TimeTag(1)` = `5` 字节；最小帧长 = `12 + 5 = 17` 字节（因此 `FORMAL_MAX_LEN = 24` 下 `set` 与 `release` 均可达）。
  - `dlt698_encode_action_response`：`87 | 01 | PIID(1) | OMD(4) | DAR(1) | DataOPT(1) | [data] | Follow(1) | TimeTag(1)` = `11 + dlen` 字节。
- **缺陷修复**：证明前核查发现上述 4 个编码器均缺少 `_fits(...)` 尾空间守卫，已补齐（`set=10u`、`connect=75u`、`release=5u`、`action=11u + dlen`）。

## 2. 合并树实测结果（`feat/dcu`）
- **Clean Release Build + Warnings**：`check_build_warnings.py` 0 warning。
- **目标回归**：`ctest --test-dir build --output-on-failure -R 'dcu|dlt698'` — **47/47 passed**。
- **Manifest 校验**：`formal_manifest.py` — **22 row(s) in 2 shard(s)** 通过。
- **CBMC `proof` / `cover` / `function` / 负向变异**：全部通过（`proof` 0 failed、`cover` SATISFIED、`responses_reject` 0 failed、负向变异脚本退出 0）。
- **守卫自检**：`run_guard_selftest.py` 全部通过。
- **结论**：`VERIFIED`。
