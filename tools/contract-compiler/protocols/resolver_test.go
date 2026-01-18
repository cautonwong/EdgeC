package protocols

import (
	"contract-compiler/model"
	"testing"
)

func TestResolve_Modbus(t *testing.T) {
	// 1. 构造测试输入
	tags := []model.TagDef{
		{
			ID:   1001,
			Name: "Voltage_A",
			Bindings: []model.Binding{
				{
					ProtocolType: "MODBUS",
					Attributes: map[string]interface{}{
						"address": 10,  // 测试 key 归一化 (addr vs address)
						"scaler":  0.1, // 测试类型转换
						"length":  2,
					},
				},
			},
		},
		{
			ID:   1002,
			Name: "Power",
			Bindings: []model.Binding{
				{
					ProtocolType: "modbus", // 测试大小写不敏感
					Attributes: map[string]interface{}{
						"addr": "0x20", // 测试 Hex 字符串解析
					},
				},
			},
		},
	}

	// 2. 执行解析
	resolved := Resolve(tags)

	// 3. 验证结果
	modbusBindings, ok := resolved["MODBUS"]
	if !ok {
		t.Fatal("Expected MODBUS bindings")
	}

	if len(modbusBindings) != 2 {
		t.Fatalf("Expected 2 bindings, got %d", len(modbusBindings))
	}

	// 验证 Item 1 (Voltage_A)
	b1 := modbusBindings[0] // 由于 Resolve 内部做了排序，Addr 10 排第一
	if b1.TagName != "Voltage_A" {
		t.Errorf("Expected Voltage_A, got %s", b1.TagName)
	}
	if b1.Addr != 10 {
		t.Errorf("Expected Addr 10, got %d", b1.Addr)
	}
	if b1.Scale != 0.1 {
		t.Errorf("Expected Scale 0.1, got %f", b1.Scale)
	}
	if b1.ByteLen != 2 {
		t.Errorf("Expected ByteLen 2, got %d", b1.ByteLen)
	}

	// 验证 Item 2 (Power) - Hex 解析和默认值
	b2 := modbusBindings[1]
	if b2.TagName != "Power" {
		t.Errorf("Expected Power, got %s", b2.TagName)
	}
	if b2.Addr != 32 { // 0x20 = 32
		t.Errorf("Expected Addr 32 (0x20), got %d", b2.Addr)
	}
	if b2.ByteLen != 2 { // Modbus 默认长度
		t.Errorf("Expected default ByteLen 2, got %d", b2.ByteLen)
	}
}

func TestResolve_UnknownProtocol(t *testing.T) {
	tags := []model.TagDef{
		{
			ID: 999,
			Bindings: []model.Binding{
				{ProtocolType: "ALIEN_TECH"},
			},
		},
	}
	resolved := Resolve(tags)
	if len(resolved) != 0 {
		t.Errorf("Expected empty result for unknown protocol")
	}
}
