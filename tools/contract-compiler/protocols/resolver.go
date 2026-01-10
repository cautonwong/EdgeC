package protocols

import (
	"contract-compiler/model"
	"fmt"
	"sort"
	"strconv"
	"strings"
)

type ResolvedBinding struct {
	TagName      string
	TagID        uint32
	ProtocolType string
	Addr         uint16
	OBIS         string
	ClassID      uint16
	AttrID       uint8
	OAD          string
	DI           string
	IOA          int
	IecType      uint8   // [新增] ASDU TypeID
	Flags        uint32  // [新增] 行为掩码 (如 FLAG_SOE)
	DeadBand     float32 // [新增] 死区阈值
	GroupID      uint8   // [新增] 聚合组 ID
	CanID        uint32
	StartBit     int
	Length       int
	ByteLen      uint16
	Access       uint32
	Factor       float32
	Offset       float32
	Scale        float32
}

func Resolve(tags []model.TagDef) map[string][]ResolvedBinding {
	res := make(map[string][]ResolvedBinding)

	for _, t := range tags {
		for _, b := range t.Bindings {
			pt := strings.ToUpper(b.ProtocolType)
			handler := GetHandler(pt)
			if handler == nil {
				continue
			}

			rb := handler.Resolve(t, b)

			// --- 通用元数据注入 ---
			rb.Access = getUint32Case(b.Attributes, "access", 0x01)
			rb.ByteLen = getUint16Case(b.Attributes, "length")
			if rb.ByteLen == 0 {
				if pt == "MODBUS" {
					rb.ByteLen = 2
				}
				if pt == "DLT645" {
					rb.ByteLen = 4
				}
			}

			res[pt] = append(res[pt], rb)
		}
	}

	if mbs, ok := res["MODBUS"]; ok {
		sort.Slice(mbs, func(i, j int) bool { return mbs[i].Addr < mbs[j].Addr })
	}
	return res
}

// --- 鲁棒性辅助函数 ---

func getUint8Case(m map[string]interface{}, key string) uint8 {
	v, ok := m[key]
	if !ok {
		v, ok = m[strings.ToUpper(key)]
	}
	if ok {
		switch val := v.(type) {
		case int:
			return uint8(val)
		case float64:
			return uint8(val)
		}
	}
	return 0
}

func getUint32Case(m map[string]interface{}, key string, def uint32) uint32 {
	v, ok := m[key]
	if !ok {
		v, ok = m[strings.ToUpper(key)]
	}
	if ok {
		switch val := v.(type) {
		case int:
			return uint32(val)
		case int32:
			return uint32(val)
		case float64:
			return uint32(val)
		case string:
			if strings.HasPrefix(val, "0x") || strings.HasPrefix(val, "0X") {
				res, _ := strconv.ParseUint(val[2:], 16, 32)
				return uint32(res)
			}
			res, _ := strconv.Atoi(val)
			return uint32(res)
		}
	}
	return def
}

func getScaler(m map[string]interface{}) float32 {
	return getFloat32Case(m, "scaler", getFloat32Case(m, "scale", 1.0))
}

func getUint16Case(m map[string]interface{}, key string) uint16 {
	v, ok := m[key]
	if !ok {
		v, ok = m[strings.ToUpper(key)]
	}
	if ok {
		switch val := v.(type) {
		case int:
			return uint16(val)
		case float64:
			return uint16(val)
		case string:
			if strings.HasPrefix(val, "0x") || strings.HasPrefix(val, "0X") {
				res, _ := strconv.ParseUint(val[2:], 16, 16)
				return uint16(res)
			}
			res, _ := strconv.Atoi(val)
			return uint16(res)
		}
	}
	return 0
}

func getStringCase(m map[string]interface{}, key string) string {
	v, ok := m[key]
	if !ok {
		v, ok = m[strings.ToUpper(key)]
	}
	if ok {
		return fmt.Sprintf("%v", v)
	}
	return ""
}

func getFloat32Case(m map[string]interface{}, key string, def float32) float32 {
	v, ok := m[key]
	if !ok {
		v, ok = m[strings.ToUpper(key)]
	}
	if ok {
		switch val := v.(type) {
		case float64:
			return float32(val)
		case int:
			return float32(val)
		case string:
			res, _ := strconv.ParseFloat(val, 32)
			return float32(res)
		}
	}
	return def
}

func getIntCase(m map[string]interface{}, key string) int {
	v, ok := m[key]
	if !ok {
		v, ok = m[strings.ToUpper(key)]
	}
	if ok {
		switch val := v.(type) {
		case int:
			return val
		case float64:
			return int(val)
		case string:
			res, _ := strconv.Atoi(val)
			return res
		}
	}
	return 0
}