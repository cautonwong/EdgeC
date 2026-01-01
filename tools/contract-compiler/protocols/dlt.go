package protocols

import (
	"contract-compiler/model"
	"strings"
)

type DltHandler struct {
	ProtoName string
}

func (h *DltHandler) Name() string { return h.ProtoName }
func (h *DltHandler) Resolve(t model.TagDef, b model.Binding) ResolvedBinding {
	rb := ResolvedBinding{
		TagName:      t.Name,
		TagID:        t.ID,
		ProtocolType: strings.ToUpper(h.ProtoName),
		Scale:        getFloat32Case(b.Attributes, "scale", 1.0),
	}
	if rb.ProtocolType == "DLT698" {
		rb.OAD = getStringCase(b.Attributes, "oad")
	} else {
		rb.DI = getStringCase(b.Attributes, "di")
	}
	return rb
}

func init() {
	Register(&DltHandler{ProtoName: "DLT645"})
	Register(&DltHandler{ProtoName: "DLT698"})
}
