package protocols

import (
	"contract-compiler/model"
)

type OpcuaHandler struct{}

func (h *OpcuaHandler) Name() string { return "OPC_UA" }
func (h *OpcuaHandler) Resolve(t model.TagDef, b model.Binding) ResolvedBinding {
	nodeid := getStringCase(b.Attributes, "nodeid")
	if nodeid == "" {
		nodeid = getStringCase(b.Attributes, "node_id")
	}
	
	// Assuming nodeid will be somehow mapped to the Address/Config string or similar
	// For basic struct matching, let's just keep the framework same. 
	// The C-side will parse the generated structures.
	return ResolvedBinding{
		TagName:      t.Name,
		TagID:        t.ID,
		ProtocolType: "OPC_UA",
		// We can reuse Addr or String fields if needed. Here we keep Scale logic intact.
		Scale:        getScaler(b.Attributes),
	}
}

func init() {
	Register(&OpcuaHandler{})
}
