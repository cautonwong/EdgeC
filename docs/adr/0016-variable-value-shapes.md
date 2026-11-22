# ADR 0016: 变量类（Class 1/3/4/6）的值形状与寻址

## 状态

Proposed（2026-11-22，待架构评审）。取代 `dcu_698_var.c` 里“所有变量都返回
float32 标量”的旧实现。

## 上下文

现实现与标准有三处偏差（`docs/design/DCU_698_OAD_STATUS.md` §4.1/§4.2）：

1. **形状**：表 125/131/134/140 规定 Class 1/3/4 的属性 2 是数组，Class 6 属性 2
   是单值；现在全部按标量返回。Class 3/4 的分相值被拆到属性 3/4/5，与标准
   “属性 2 是数组、属性 3 是 Scaler_Unit”冲突。
2. **类型**：表 A.3 的实例类型是整数（电压 long-unsigned、电流/功率 double-long、
   功率因数 long、频率 long-unsigned）；现在 2000/2001/2004 发 float32。
3. **量纲**：标准给整数 + Scaler_Unit（如电压 ×10、电流 ×1000、电能 ×100/×10⁴）；
   现在把 DataHub 的工程值直接截断，200A 0.95 变 0、200F 50.01 变 50。

另外 `dcu_698_var_read_value()` 固定读 `device_id=0`，忽略 OAD 的元素索引。

## 决策

### 值形状

| OI | 属性 | A-XDR 形状 | 元素类型 | scaler | unit（表B.1） |
| --- | --- | --- | --- | --- | --- |
| 0010/0020 | 2 | `array`，第一项总，后面费率 | double-long-unsigned [6] | −2 | 33 kWh |
| 0010/0020 | 3 | `structure{integer, enum}` | Scaler_Unit | −2 | 33 |
| 0010/0020 | 4 | `array` | long64-unsigned [21] | −4 | 33 |
| 0010/0020 | 5 | Scaler_Unit | — | −4 | 33 |
| 2000 | 2 | `array` A/B/C | long-unsigned [18] | −1 | 38 V |
| 2000 | 3 | Scaler_Unit | — | −1 | 38 |
| 2001 | 2 | `array` A/B/C | double-long [5] | −3 | 36 A |
| 2001 | 3 | Scaler_Unit | — | −3 | 36 |
| 2004 | 2 | `array` 总/A/B/C | double-long [5] | −1 | 27 W |
| 2004 | 3 | Scaler_Unit | — | −1 | 27 |
| 200A | 2 | `array`，当前只有总 | long [16] | −3 | 0（无单位） |
| 200A | 3 | Scaler_Unit | — | −3 | 0 |
| 200F | 2 | 单值（Class 6） | long-unsigned [18] | −2 | 47 Hz |
| 200F | 3 | Scaler_Unit | — | −2 | 47 |

数组元素数取产品实际测得的通道数：2000/2001 三相，2004 总加三相；200A 只有
总功率因数，数组 1 项。费率电能当前没有独立影子，Class 1 数组只有总项。

`raw = round(工程值 × 10^(-scaler))`，标量端口只承载 Class 6 的单值；数组与
Scaler_Unit 只走 `resolve_axdr`。

### 寻址

`OAD = OI(16) | 属性(8) | 元素索引(8)`。

- **元素索引 = 表槽位**（`device_id`）。同一 OI 的不同表由索引区分；索引超出
  档案/影子时返回 DAR 6。
- **相位是属性 2 数组的元素**，不是属性字节。`0x20000300` 是属性 3
  （Scaler_Unit），不是 B 相；B 相是 `0x20000200` 响应的数组第 2 项。
- 驱动里的 `OAD_VOLTAGE_B` 等常量只作为内部 DataHub tag id 和云侧 metric 名，
  不再是 698 线上 OAD；改名 `TAG_*` 以免误用。

### 编码

数组/结构由产品侧按 A-XDR 直接写入调用方缓冲（`EDGE_SLAVE_AXDR_MAX`），
无动态内存。Class 6 单值继续复用 `dcu_698_value_for_oad`。

## 后果

- 北向 GET 的字节形状变化：2000/2001/2004/0010/0020 从标量变数组；200A/200F
  变整数 + Scaler_Unit。
- 验收与主站点表必须按数组解析；标量通道保留给 Class 6。
- 采集方案要覆盖数组用到的相/总通道，否则整条读取按 DAR 6 失败，不用 0 填充。
- 3000/3005、5004、6000/6012、8000/F100/F200 不在本 ADR 范围。
