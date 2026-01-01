package protocols

import (
	"contract-compiler/model"
)

type ModbusHandler struct{}

func (h *ModbusHandler) Name() string { return "MODBUS" }
func (h *ModbusHandler) Resolve(t model.TagDef, b model.Binding) ResolvedBinding {
	return ResolvedBinding{
		TagName:      t.Name,
		TagID:        t.ID,
		ProtocolType: "MODBUS",
		Addr:         getUint16Case(b.Attributes, "addr"),
		Scale:        getFloat32Case(b.Attributes, "scale", 1.0),
	}
}

func init() {
	Register(&ModbusHandler{})
}