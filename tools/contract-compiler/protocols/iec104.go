package protocols

import (
	"contract-compiler/model"
	"strings"
)

type IecHandler struct{}

func (h *IecHandler) Name() string { return "IEC104" }

func (h *IecHandler) Resolve(t model.TagDef, b model.Binding) ResolvedBinding {
	pt := strings.ToUpper(b.ProtocolType)

	// 1. 基础地址
	ioa := getIntCase(b.Attributes, "ioa")

	// 2. 行为掩码 (Flags)
	var flags uint32 = 0
	if isTrue(b.Attributes["soe"]) {
		flags |= 0x01 // FLAG_SOE
	}

	// 3. TypeID 解析与推断
	typeID := getUint8Case(b.Attributes, "iec_type")
	if typeID == 0 {
		// 启发式：根据 TagDef 的类型推断
		switch strings.ToLower(t.Type) {
		case "bool", "int":
			if flags&0x01 != 0 {
				typeID = 30 // M_SP_TB_1
			} else {
				typeID = 1 // M_SP_NA_1
			}
		case "float", "double":
			if flags&0x01 != 0 {
				typeID = 36 // M_ME_TF_1
			} else {
				typeID = 13 // M_ME_NC_1
			}
		}
	}

	return ResolvedBinding{
		TagName:      t.Name,
		TagID:        t.ID,
		ProtocolType: pt,
		IOA:          ioa,
		IecType:      typeID,
		Flags:        flags,
		DeadBand:     getFloat32Case(b.Attributes, "dead_band", 0.0),
		GroupID:      getUint8Case(b.Attributes, "group_id"),
		Scale:        getScaler(b.Attributes),
	}
}

func isTrue(v interface{}) bool {
	if v == nil {
		return false
	}
	switch val := v.(type) {
	case bool:
		return val
	case int:
		return val != 0
	case string:
		return strings.ToLower(val) == "true" || val == "1"
	}
	return false
}

func init() {
	h := &IecHandler{}
	Register(h)
	// 归一化处理：101 与 104 共享解析逻辑
	registry["IEC101"] = h
}