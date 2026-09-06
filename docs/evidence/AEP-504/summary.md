# AEP-504 Delivery Summary: 工业控制台安全门禁、高危操作双人复核与全量审计留痕

## 1. 架构目标与标准对齐 (IEC 62443 SL-3 & 电力二次系统安全防护)

在 EdgeOS 全局 Web 控制台（直通底层 DataHub POSIX SHM、Cyclone DDS 实时网格、南向驱动与流计算引擎）中，实现工业级强制安全门禁：
1. **RBAC 角色权限硬隔离**:
   - `Observer` (监视员)：完全隐藏/禁用 Web 控制台入口；
   - `Operator` (运行员)：仅限执行只读/诊断命令（如 `datahub stats`, `tag read`, `dds peers`, `storage wal`），写/配置/控制指令在输入端即时告警阻断（返回 403 / -EPERM）；
   - `Engineer / Admin` (工程师/管理员)：允许全量调试命令执行，高危指令自动触发双人复核拦截。
2. **高危操作模式识别与双人复核拦截 (Two-Man Rule Interlock)**:
   - 前后端统一建立高危指令黑名单正则（`reboot`, `poweroff`, `tag write`, `pipeline stop`, `wal format`, `param apply`, `do write`, `relay trip` 等）；
   - 触发高危指令时自动弹出 **“高危控制操作双人复核 / 物理二次确认”** 模态框 (`TwoManRuleModal.vue`)；
   - 强制校验第二监护人凭证（禁止自审，支持 6 位 TOTP 动态口令或专属 Master Passkey），并具备 60 秒超时自动作废防挂起机制。
3. **全量 WAL 审计追踪 (Audit Trail)**:
   - 所有在 Web 控制台执行的指令、执行者用户名、用户角色、客户端 IP、MTT 时间戳、双人复核状态、第二复核人、执行状态码一律写入只读 WAL 审计日志；
   - 记录级 CRC32 / 密码学校验码封签，提供防篡改扫描与 JSON 导出接口。

---

## 2. 交付物矩阵

### 2.1 C 核心安全引擎 (`modules/edge_mgmt/`)
- `modules/edge_mgmt/include/edgeos/console_rbac.h`：工业 RBAC 角色、风险等级判定与双人复核校验接口；
- `modules/edge_mgmt/src/auth/console_rbac.c`：RBAC 权限判定逻辑与 Two-Man Rule 验证器；
- `modules/edge_mgmt/include/edgeos/console_audit.h`：WAL 审计日志结构、记录接口、查询与防篡改验证接口；
- `modules/edge_mgmt/src/audit/console_audit.c`：CRC32 封签计算、环形持久化审计日志存储与 JSON 导出；
- `modules/edge_mgmt/CMakeLists.txt`：注册编译 `console_rbac.c` 与 `console_audit.c`。

### 2.2 前端安全组件 (`web/core/`)
- `web/core/src/components/console/TwoManRuleModal.vue`：工业警示级双人复核模态框，支持 60s 倒计时、自审防护拦截与凭据确认；
- `web/core/src/components/console/InteractiveShell.vue`：集成 RBAC 角色切换、Operator 写操作阻断、高危指令拦截弹窗、`auth whoami` 与 `audit log` CLI 指令；
- `web/core/src/index.ts`：导出 `TwoManRuleModal` 组件。

### 2.3 验证套件与存盘证据
- `tests/test_web_console_rbac_audit.c`：42/42 PASS（覆盖角色解析、门禁拦截、高危判定、双人复核、审计存储、CRC32 防篡改与 JSON 导出）；
- `web/tests/run_rbac_e2e.js` 与 `test_rbac_two_man_rule.spec.js`：13/13 PASS（覆盖全链路无缝自动化操作与 8 张存盘截图）；
- `docs/evidence/AEP-504/`：8 张 E2E 全景截图。
