# AEP-004 证据包总结 (Summary)

- **工单 ID**: AEP-004
- **标题**: R3 未闭合项收口（T-002 剩余 UDS/HMAC/DataHub）
- **状态**: Done
- **完成时间**: 2026-08-16

---

## 1. 变更内容与改动说明

1. **HMAC 密钥外置**: 在 `go/cmd/smart-box-gateway/main.go` 中，`computeSecurityKey` 读取环境变量 `DOIP_UDS_SECRET_KEY`，在生产模式下未配置直接阻断（Fail-Safe），在非生产模式下自动生成 32 字节随机临时密钥并输出警告。
2. **DataHub UDS 状态回写**: UDS 会话状态（`UDS_SESSION_STATE`）、安全解锁状态（`UDS_SECURITY_UNLOCKED`）、活动 DTC 计数（`UDS_ACTIVE_DTC_COUNT`）和 Routine 状态（`UDS_ROUTINE_STATUS`）通过 `g.recordTagValue()` 实时写入 DataHub 影子内存。
3. **消除硬编码与防暴力破解**: 引入 `secAccessManager` 追踪客户端 IP，连续 3 次密钥错误触发 10 秒时间锁；动态返回 DTC 与 ECU 名称（可通过 `ECU_NAME` 覆盖）。
4. **单元测试全量覆盖**: `go/cmd/smart-box-gateway/main_test.go` 中 `TestUDSSecurityHardeningAndRBAC` 覆盖会话切换、种子请求、密钥解锁、防暴力破解锁定与 5 大 UDS 服务的完整通信流。

---

## 2. DoD 逐项验收勾选

- [x] `go/cmd/smart-box-gateway/main_test.go` 新增用例全绿
- [x] `rg` 扫描代码无硬编码密钥
- [x] UDS 状态经 DataHub 写入契约 Tag
- [x] 无 R-01~R-14 红线违规
