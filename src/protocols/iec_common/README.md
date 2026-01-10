# IEC 60870-5 Common Core (ASDU) 深度指南

## 1. 协议核心：ASDU (应用服务数据单元)

IEC 101 和 IEC 104 共享应用层（Application Layer）。ASDU 是承载电力业务数据的核心结构。

### 1.1 帧结构
`[TypeID] [SQ + Count] [COT] [CASDU] [IOA] [DATA]`

*   **TypeID (1B)**: 类型标识 (如 M_SP_NA_1 单点遥信)。
*   **SQ (Bit 7 of Byte 2)**: 结构限定词。
    *   `SQ=0` (Sequence of Objects): `[IOA1, Val1], [IOA2, Val2]...` (离散地址，灵活但低效)。
    *   `SQ=1` (Sequence of Elements): `[Start_IOA], [Val1], [Val2]...` (连续地址，紧凑高效，用于 GI)。
*   **Count (Bits 0-6 of Byte 2)**: 数据对象数量 (Max 127)。
*   **COT (Cause of Transmission)**: 传输原因 (1B 或 2B)。
    *   `3`: 突发 (Spontaneous) - 最常用。
    *   `20`: 总召唤 (GI)。
    *   `6/7`: 激活/激活确认 (遥控)。
*   **CASDU (Common Address)**: 公共地址 (1B 或 2B)，即设备地址。
*   **IOA (Information Object Address)**: 信息体地址 (1B, 2B 或 3B)，即点位 ID。

### 1.2 关键数据类型 (TypeID)

| TypeID | 符号 | 描述 | 应用场景 |
| :--- | :--- | :--- | :--- |
| **1** | M_SP_NA_1 | 单点信息 | 开关分/合，无时标 |
| **3** | M_DP_NA_1 | 双点信息 | 隔离开关/断路器 (分/合/中间/故障) |
| **9** | M_ME_NA_1 | 归一化值 | 交流采样 (-1.0 ~ 1.0) |
| **13** | M_ME_NC_1 | 短浮点数 | IEEE 754 浮点 (最常用) |
| **30** | M_SP_TB_1 | 带 CP56Time2a 单点 | SOE 事件记录 (必须带时标) |
| **45** | C_SC_NA_1 | 单命令 | 遥控 (起/停) |
| **100** | C_IC_NA_1 | 总召唤 | 初始化全量同步 |

---

## 2. 契约驱动落地现状 (Status)

### 已完成 (Done)
- [x] **归一化编解码器**: `iec_asdu_codec.c` 实现了对 Type 1, 13, 45, 100 等核心类型的解析与构建。
- [x] **动态地址长度**: 通过 `iec_addr_config_t` 支持 101/104 不同的 COT/CASDU/IOA 长度配置。
- [x] **契约注入**: `contract-compiler` 已支持注入 `iec_type`, `dead_band`, `group_id` 等行为元数据。
- [x] **万能填充接口**: `iec_asdu_add_any` 实现了基于 TypeID 的盲执行逻辑。

### 待开发 (TODO List)

#### 2.1 数据类型补全
- [ ] **CP56Time2a 支持**: 完善 `iec_cp56time.c`，支持 7 字节时标的毫秒级精度解析与生成。
- [ ] **双点信息 (DP)**: 实现 Type 3 (M_DP_NA_1) 和 Type 31 (M_DP_TB_1)。双点处理涉及 `00(中间), 01(分), 10(合), 11(故障)` 状态逻辑。
- [ ] **遥控反校 (Check-back)**: 在处理 C_SC_NA_1 (45) 时，实现 Select/Execute 两阶段状态机校验。

#### 2.2 高级特性
- [ ] **SQ=1 自动成组**: 目前驱动层主要使用 SQ=0。需要实现：当检测到连续 IOA 时，自动切换为 SQ=1 打包发送，极大提升 GI 效率。
- [ ] **死区算法 (Deadband)**: 虽然契约已注入 `dead_band`，但需要在 `driver_iec104.c` 中完善浮点数比较逻辑 (Integration/Accumulator)。

#### 2.3 安全扩展
- [ ] **IEC 62351-5**: 预研应用层安全，为 ASDU 增加 HMAC 签名预留接口。
