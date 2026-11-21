# DCU-D0-05a 校验 698 HCS

## 做了什么

`dlt698_decode_frame_ex` 原来只校验 FCS，不校验 HCS（标准 5.1.5）。本次补上 HCS 校验，并把 D0-05 里以 skip 记录的反例改为强断言。

## HCS 范围

`68` 之后到 HCS 之前的字节：长度域(2) + 控制码(1) + 地址域(1 + SA + CA)，即 `5 + sa_len` 字节，其中 `sa_len = (SA 配置字节 & 0x0F) + 1`。

- 编码端 `_frame_seal` 原来按指针差值算这一段，语义相同。
- 本次把公式抽成 `_hcs_cover_len(sa_len)` 与常量 `DLT698_HCS_COVER_FIXED`，解码端直接复用，避免两处各写一遍。
- 校验算法复用 codec 现有的 `dlt698_calculate_fcs`（附录 D 的 PPP FCS-16），没有新写多项式。

## 解码顺序

`dlt698_decode_frame_ex` 在解 APDU 之前依次校验：起止字符 → 长度域 → 地址域长度 → **HCS** → FCS。HCS 不匹配返回 -1。HCS 的偏移量同时用于定位 APDU 起点，删掉了原来单独推导的 `header_len`。

## 测试

- `test_reject_bad_hcs` 从 skip 改为断言：改错 HCS 后**重算 FCS**，让帧只剩 HCS 一处错，解析必须返回 -1。只改 HCS 不重算 FCS 的话，FCS 会先失败，那条证明不了 HCS 被检查。
- 合法帧继续通过：21 个用例，20 通过、1 跳过（仅剩 GET-Record/RSD，指向 D1-15）。
- 标准帧向量未改。

## 验证

| 命令 | 结果 |
| --- | --- |
| `cmake --build build -j8` | 成功 |
| `cd build && ctest --output-on-failure -R dlt698` | 6/6 通过（`runtime.log`） |
| `cd build && ctest -j8 -E '^app_platform_e2e$'` | 329/329 通过，无回归 |
| `./build/tests/test_dlt698_conformance_suite` | 21 用例，20 通过、1 跳过（`tests.log`） |
| `python3 ci/scripts/run_guard_selftest.py --root .` | 全部守卫与正反夹具通过 |

## 改动范围

`foundation/src/protocol/dlt698_codec.c`、`tests/test_dlt698_conformance_suite.c`、`docs/evidence/DCU-D0-05a/`。未改产品代码，未改标准帧向量。
