package protocols

import (
	"contract-compiler/model"
)

type Iec104Handler struct{}

func (h *Iec104Handler) Name() string { return "IEC104" }
func (h *Iec104Handler) Resolve(t model.TagDef, b model.Binding) ResolvedBinding {
	return ResolvedBinding{
		TagName:      t.Name,
		TagID:        t.ID,
		ProtocolType: "IEC104",
		IOA:          getIntCase(b.Attributes, "ioa"),
	}
}

func init() {
	Register(&Iec104Handler{})
}