package core

import (
	"fmt"
	"strconv"
	"strings"
)

// ProtocolHashInfo 存储生成 C 代码所需的哈希常数
type ProtocolHashInfo struct {
	Seed      uint32
	TableSize uint32
	Keys      []HashEntry
	IsPerfect bool
}

type HashEntry struct {
	RawKey   uint64 // 原始数值键 (DI, OAD, Addr, etc.)
	StrKey   string // 原始字符串键 (OBIS, etc.)
	TagID    uint32
	TagName  string
	Scale    float32
	ByteLen  uint16 // [补强]
	Access   uint32 // [补强]
	Index    uint32 // [新增] 在物理哈希表中的索引位
}

// ParseProtocolKey 将不同协议的标识符解析为统一的 uint64 键值
func ParseProtocolKey(proto, key string) uint64 {
	switch proto {
	case "MODBUS", "IEC104":
		v, _ := strconv.ParseUint(key, 0, 16)
		return v
	case "DLT645", "DLT698":
		v, _ := strconv.ParseUint(strings.ReplaceAll(key, "0x", ""), 16, 32)
		return v
	case "DLMS":
		// 支持 OBIS (1.0.1.8.0.255) 或带 Attribute ID 的格式 (1.0.1.8.0.255:2)
		cleanKey := key
		if strings.Contains(key, ":") {
			cleanKey = strings.Split(key, ":")[0]
		}
		parts := strings.Split(cleanKey, ".")
		var res uint64
		for i, p := range parts {
			if i >= 8 { break }
			v, _ := strconv.Atoi(p)
			res |= uint64(uint8(v)) << (i * 8)
		}
		return res
	default:
		return 0
	}
}

func computeHash(key uint64, seed uint32, size uint32) uint32 {
	h := uint32(key) ^ seed
	h ^= h >> 16
	h *= 0x85ebca6b
	h ^= h >> 13
	h *= 0xc2b2ae35
	h ^= h >> 16
	return h & (size - 1)
}

func GeneratePerfectHash(proto string, entries []HashEntry) ProtocolHashInfo {
	if len(entries) == 0 {
		return ProtocolHashInfo{}
	}

	keyMap := make(map[uint64]string)
	for _, e := range entries {
		if other, exists := keyMap[e.RawKey]; exists && other != e.TagName {
			panic(fmt.Sprintf("FATAL CONTRACT ERROR [%s]: Physical Key Collision! Address/DI '%d' is assigned to both '%s' and '%s'.", proto, e.RawKey, other, e.TagName))
		}
		keyMap[e.RawKey] = e.TagName
	}

	size := uint32(1)
	for size < uint32(len(entries)) { size <<= 1 }
	if size < 4 { size = 4 }

	for attempt := 0; attempt < 8; attempt++ {
		for seed := uint32(0); seed < 1000000; seed++ {
			occupied := make(map[uint32]bool)
			collision := false
			for _, e := range entries {
				h := computeHash(e.RawKey, seed, size)
				if occupied[h] { collision = true; break }
				occupied[h] = true
			}
			if !collision {
				table := make([]HashEntry, size)
				for _, e := range entries {
					h := computeHash(e.RawKey, seed, size)
					e.Index = h
					table[h] = e
				}
				return ProtocolHashInfo{Seed: seed, TableSize: size, Keys: table, IsPerfect: true}
			}
		}
		size <<= 1
	}
	return ProtocolHashInfo{Seed: 0, TableSize: uint32(len(entries)), Keys: entries, IsPerfect: false}
}