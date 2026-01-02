package protocols

import (
	"contract-compiler/model"
)

type ModbusHandler struct{}

func (h *ModbusHandler) Name() string { return "MODBUS" }
func (h *ModbusHandler) Resolve(t model.TagDef, b model.Binding) ResolvedBinding {
	addr := getUint16Case(b.Attributes, "addr")
	if addr == 0 {
		addr = getUint16Case(b.Attributes, "address")
	}
	
	return ResolvedBinding{
		TagName:      t.Name,
		TagID:        t.ID,
		ProtocolType: "MODBUS",
		Addr:         addr,
		Scale:        getScaler(b.Attributes),
	}
}

func init() {
	Register(&ModbusHandler{})
}