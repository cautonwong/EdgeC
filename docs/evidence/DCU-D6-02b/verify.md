# DCU-D6-02b 独立验证报告

## 1. 独立预期推导
- **标准依据**：DL/T 698.45 5.1.5（HCS）、5.1.7（FCS）、表 60（`0x82` CONNECT-Response）、表 62（`0x83` RELEASE-Response）、表 67（RCSD）、表 79/87（SET/ACTION）。
- **独立期望值**：
  - 破坏 HCS 或 FCS 校验字节的请求帧应被 `dlt698_decode_frame_ex` 拒绝（返回 `-1`）且从站驱动丢弃；
  - 截断的 SET/ACTION APDU 应返回 DAR 7（`DLT698_DAR_TYPE_UNMATCHED`）或被拒绝而不越界；
  - `n > DLT698_RCSD_MAX` 的 `GetRecord` 响应编码应返回 `-1`（`FORMAL-05` 回归）；不支持的 RSD Selector 3/4/5 应返回 DAR 拒绝；
  - `CONNECT-Response`（`0x82`）与 `RELEASE-Response`（`0x83`）控制域 `0xC3`、HCS/FCS 校验应合法。

## 2. 合并树实测结果（`feat/dcu`）
- **Clean Release Build + Warnings**：`check_build_warnings.py` 0 warning。
- **目标回归**：`test_dcu_698_wire_vectors`（17 例）与 `ctest -R 'dcu|dlt698'`（**47/47**）全部通过。
- **ASAN**：`test_dcu_698_wire_vectors` — **1/1 passed**，0 泄漏/越界。
- **结论**：`VERIFIED`。
