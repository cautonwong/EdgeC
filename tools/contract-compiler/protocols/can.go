package protocols

import (
	"contract-compiler/model"
)

type CanHandler struct{}

func (h *CanHandler) Name() string { return "CAN" }
func (h *CanHandler) Resolve(t model.TagDef, b model.Binding) ResolvedBinding {
	rb := ResolvedBinding{
		TagName:      t.Name,
		TagID:        t.ID,
		ProtocolType: "CAN",
	}
	a := b.Attributes
	rb.CanID = uint32(getIntCase(a, "can_id"))
	rb.StartBit = getIntCase(a, "start")
	if rb.StartBit == 0 { rb.StartBit = getIntCase(a, "start_bit") }
	rb.Length = getIntCase(a, "len")
	if rb.Length == 0 { rb.Length = getIntCase(a, "length") }
	rb.Factor = getFloat32Case(a, "factor", 1.0)
	rb.Offset = getFloat32Case(a, "offset", 0.0)
	return rb
}

func init() {
	Register(&CanHandler{})
}