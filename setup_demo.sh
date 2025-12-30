#!/bin/bash

# 1. 创建目录结构
mkdir -p contracts/schema
mkdir -p contracts/bindings/{modbus,dlms,iec104,can}
mkdir -p contracts/products
mkdir -p tools
mkdir -p generated/{include,src}
mkdir -p cmd/hems
mkdir -p cmd/sg_el2025

echo "[1/5] Creating Directory Structure... Done."

# ---------------------------------------------------------
# 2. 全局语义定义 (Schema)
# ---------------------------------------------------------
cat > contracts/schema/tags.yaml <<EOF
tags:
  # === 电力遥测 (1000-1999) ===
  - id: 1001
    name: VOLTAGE_PHASE_A
    type: float
    unit: V
    desc: "A相电压"

  - id: 1002
    name: CURRENT_PHASE_A
    type: float
    unit: A
    desc: "A相电流"

  # === 能量计量 (3000-3999) ===
  - id: 3001
    name: ENERGY_ACTIVE_IMPORT
    type: double
    unit: kWh
    desc: "正向有功总电能"
    attributes: { persist: true, upload: true }

  # === 电池管理 (4000-4999) ===
  - id: 4001
    name: BATTERY_SOC
    type: float
    unit: "%"
    desc: "电池剩余电量"
  
  - id: 4002
    name: BATTERY_TEMP_MAX
    type: float
    unit: "C"
    desc: "电芯最高温度"

  # === 开关控制 (5000-5999) ===
  - id: 5001
    name: SWITCH_RELAY_1
    type: int
    desc: "1号继电器状态"
    attributes: { rw: "w" }
EOF

# ---------------------------------------------------------
# 3. 协议绑定库 (Bindings)
# ---------------------------------------------------------

# 3.1 Modbus: 华为逆变器
cat > contracts/bindings/modbus/huawei_sun2000.yaml <<EOF
model: "Huawei_SUN2000"
protocol: "MODBUS"
mappings:
  - tag: VOLTAGE_PHASE_A
    addr: 30001
    len: 1
    type: uint16
    scale: 0.1
  - tag: CURRENT_PHASE_A
    addr: 30002
    len: 1
    type: uint16
    scale: 0.01
EOF

# 3.2 DLMS: 正泰电表
cat > contracts/bindings/dlms/chint_dtsu.yaml <<EOF
model: "Chint_DTSU666"
protocol: "DLMS"
mappings:
  - tag: VOLTAGE_PHASE_A
    obis: "1.0.32.7.0.255"
    class_id: 3
    attr_id: 2
    scaler: 0.1
  - tag: ENERGY_ACTIVE_IMPORT
    obis: "1.0.1.8.0.255"
    class_id: 3
    attr_id: 2
    scaler: 1.0
EOF

# 3.3 IEC 104: 综保装置
cat > contracts/bindings/iec104/nari_relay.yaml <<EOF
model: "Nari_PCS_9600"
protocol: "IEC104"
mappings:
  - tag: SWITCH_RELAY_1
    ioa: 2001
    type_id: 45 # C_SC_NA_1 (单点命令)
  - tag: CURRENT_PHASE_A
    ioa: 16385
    type_id: 36 # M_ME_TF_1 (短浮点)
EOF

# 3.4 CAN: 宁德时代 BMS
cat > contracts/bindings/can/catl_bms.yaml <<EOF
model: "CATL_Rack_V1"
protocol: "CAN"
mappings:
  - tag: BATTERY_SOC
    can_id: 0x18FF50E5
    start_bit: 0
    length: 8
    factor: 0.4
    offset: 0
  - tag: BATTERY_TEMP_MAX
    can_id: 0x18FF50E5
    start_bit: 16
    length: 8
    factor: 1.0
    offset: -40
EOF

echo "[3/5] Generating Bindings... Done."

# ---------------------------------------------------------
# 4. 产品定义 (Products)
# ---------------------------------------------------------

# 产品 A: SG-EL2025 (全能型)
cat > contracts/products/sg_el2025.yaml <<EOF
product: "SG_Fusion_Terminal"
includes:
  - "contracts/bindings/dlms/chint_dtsu.yaml"
  - "contracts/bindings/iec104/nari_relay.yaml"
  - "contracts/bindings/can/catl_bms.yaml"
overrides:
  BATTERY_SOC:
    upload: true
    persist: false # 实时变化，只上报不存盘
EOF

# 产品 B: HEMS (Modbus only)
cat > contracts/products/hems.yaml <<EOF
product: "Home_Gateway_Lite"
includes:
  - "contracts/bindings/modbus/huawei_sun2000.yaml"
overrides:
  VOLTAGE_PHASE_A:
    upload: false # 节省流量
EOF

echo "[4/5] Generating Products... Done."
echo "Setup Complete. Run 'python3 tools/contract_compiler.py sg_el2025' to test."