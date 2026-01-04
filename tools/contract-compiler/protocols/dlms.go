package protocols

import (
	"contract-compiler/model"
)

type DlmsHandler struct{}

func (h *DlmsHandler) Name() string { return "DLMS" }
func (h *DlmsHandler) Resolve(t model.TagDef, b model.Binding) ResolvedBinding {
	return ResolvedBinding{
		TagName:      t.Name,
		TagID:        t.ID,
		ProtocolType: "DLMS",
		OBIS:         getStringCase(b.Attributes, "obis"),
		ClassID:      getUint16Case(b.Attributes, "class_id"),
		AttrID:       getUint8Case(b.Attributes, "attr_id"),
		Scale:        getScaler(b.Attributes),
	}
}

func init() {
	Register(&DlmsHandler{})
}