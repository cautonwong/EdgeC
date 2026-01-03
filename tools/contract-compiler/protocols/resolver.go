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
	OAD          string
	DI           string
	IOA          int
	CanID        uint32
	StartBit     int
	Length       int
	Factor       float32
	Offset       float32
	Scale        float32
}

func Resolve(tags []model.TagDef) map[string][]ResolvedBinding {
	res := make(map[string][]ResolvedBinding)

	for _, t := range tags {
		for _, b := range t.Bindings {
			pt := strings.ToUpper(b.ProtocolType)
			fmt.Printf("[Resolver] Tag: %s, Found Protocol: %s\n", t.Name, pt)
			handler := GetHandler(pt)
			if handler == nil { 
				fmt.Printf("[Resolver] WARNING: No handler for protocol %s\n", pt)
				continue 
			}

			rb := handler.Resolve(t, b)
			res[pt] = append(res[pt], rb)
		}
	}

	// 排序逻辑 (物理一致性保证)
	if mbs, ok := res["MODBUS"]; ok {
		sort.Slice(mbs, func(i, j int) bool { return mbs[i].Addr < mbs[j].Addr })
	}
	return res
}

// --- 鲁棒性辅助函数 (共享) ---

func getScaler(m map[string]interface{}) float32 {
	return getFloat32Case(m, "scaler", getFloat32Case(m, "scale", 1.0))
}

func getUint16Case(m map[string]interface{}, key string) uint16 {
	v, ok := m[key]
	if !ok { v, ok = m[strings.ToUpper(key)] }
	if ok {
		switch val := v.(type) {
		case int: return uint16(val)
		case int32: return uint16(val)
		case float64: return uint16(val)
		case string:
			// 处理 0x 十六进制字符串
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
	if !ok { v, ok = m[strings.ToUpper(key)] }
	if ok { return fmt.Sprintf("%v", v) }
	return ""
}

func getFloat32Case(m map[string]interface{}, key string, def float32) float32 {
	v, ok := m[key]
	if !ok { v, ok = m[strings.ToUpper(key)] }
	if ok {
		switch val := v.(type) {
		case float64: return float32(val)
		case int: return float32(val)
		case string:
			res, _ := strconv.ParseFloat(val, 32)
			return float32(res)
		}
	}
	return def
}

func getIntCase(m map[string]interface{}, key string) int {
	v, ok := m[key]
	if !ok { v, ok = m[strings.ToUpper(key)] }
	if ok {
		switch val := v.(type) {
		case int: return val
		case int32: return int(val)
		case float64: return int(val)
		case string:
			if strings.HasPrefix(val, "0x") || strings.HasPrefix(val, "0X") {
				res, _ := strconv.ParseInt(val[2:], 16, 32)
				return int(res)
			}
			res, _ := strconv.Atoi(val)
			return res
		}
	}
	return 0
}