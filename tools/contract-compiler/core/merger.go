package core

import (
	"contract-compiler/model"
	"fmt"
	"os"
	"path/filepath"
	"sort"
	"strings"
	"gopkg.in/yaml.v3"
)

func BuildContract(schemaPath, productPath, localPath, baseDir string) *model.EffectiveContract {
	// rawTags 存储测点的最原始 map 数据，用于层层合并
	rawTags := make(map[string]map[string]interface{})
	enabledSet := make(map[string]bool)

	// 1. Foundation (最低优先级)
	loadYamlToRawMap(schemaPath, "tags", rawTags)

	// 2. Includes (库映射 - 中优先级)
	var pc model.ProductConfig
	pData, _ := os.ReadFile(productPath)
	yaml.Unmarshal(pData, &pc)
	
	for _, inc := range pc.Includes {
		absPath := inc
		if !filepath.IsAbs(inc) && baseDir != "" { absPath = filepath.Join(baseDir, inc) }
		if bData, err := os.ReadFile(absPath); err == nil {
			var raw map[string]interface{}
			yaml.Unmarshal(bData, &raw)
			proto, _ := raw["protocol"].(string)
			mList, _ := raw["mappings"].([]interface{})
			if len(mList) == 0 { mList, _ = raw["map"].([]interface{}) }
			for _, m := range mList {
				if mm, ok := m.(map[string]interface{}); ok {
					if tagName, ok := mm["tag"].(string); ok {
						injectBinding(rawTags, tagName, proto, mm)
					}
				}
			}
		}
	}

	// 3. Product Overrides (高优先级 - 覆盖库文件)
	// 注意：在此阶段我们先处理 product.yaml 里的 enabled_tags
	for _, n := range pc.EnabledTags { enabledSet[n] = true }
	
	for name, ovr := range pc.Overrides {
		if tag, ok := rawTags[name]; ok {
			// 将 Overrides 里的字段注入到原始 map
			b, _ := yaml.Marshal(ovr)
			var ovrMap map[string]interface{}
			yaml.Unmarshal(b, &ovrMap)
			for k, v := range ovrMap {
				if v != nil { tag[k] = v }
			}
		}
	}

	// 4. Local Extension (最高优先级 - 现场真相)
	if localPath != "" {
		if lData, err := os.ReadFile(localPath); err == nil {
			var local map[string]interface{}
			yaml.Unmarshal(lData, &local)
			// 使能现场定义的测点
			if en, ok := local["enabled_tags"].([]interface{}); ok {
				for _, n := range en { if s, ok := n.(string); ok { enabledSet[s] = true } }
			}
			// 注入现场 Tags (物理地址覆盖发生在这里)
			if tags, ok := local["tags"].([]interface{}); ok {
				for _, t := range tags {
					if tm, ok := t.(map[string]interface{}); ok {
						if name, ok := tm["name"].(string); ok {
							// 仅执行合并，不再强制 enabledSet[name] = true
							mergeMap(rawTags, name, tm) 
						}
					}
				}
			}
		}
	}

	// 5. Final Assembly & Assembly-time Validation
	var activeTags []model.TagDef
	var allTags []model.TagDef
	usedIDs := make(map[uint32]string)
	
	for name, rm := range rawTags {
		var t model.TagDef
		b, _ := yaml.Marshal(rm)
		yaml.Unmarshal(b, &t)
		
		if t.ID != 0 {
			if existingName, conflict := usedIDs[t.ID]; conflict && existingName != name {
				panic(fmt.Sprintf("FATAL: ID Conflict! ID %d is used by both '%s' and '%s'", t.ID, existingName, name))
			}
			usedIDs[t.ID] = name
		}

		allTags = append(allTags, t)
		if enabledSet[name] { activeTags = append(activeTags, t) }
	}

	validateProtocolOverlap(activeTags)

	sort.Slice(activeTags, func(i, j int) bool { return activeTags[i].ID < activeTags[j].ID })
	sort.Slice(allTags, func(i, j int) bool { return allTags[i].ID < allTags[j].ID })

	prodName := pc.ProductName
	if prodName == "" {
		var raw map[string]interface{}
		yaml.Unmarshal(pData, &raw)
		if n, ok := raw["product"].(string); ok { prodName = n }
	}

	return &model.EffectiveContract{ Product: prodName, ActiveTags: activeTags, AllTags: allTags }
}

// 辅助函数保持逻辑一致
func loadYamlToRawMap(path, key string, target map[string]map[string]interface{}) {
	data, err := os.ReadFile(path); if err != nil { return }
	var raw map[string]interface{}; yaml.Unmarshal(data, &raw)
	if list, ok := raw[key].([]interface{}); ok {
		for _, item := range list {
			if m, ok := item.(map[string]interface{}); ok {
				if name, ok := m["name"].(string); ok { target[name] = m }
			}
		}
	}
}

func mergeMap(target map[string]map[string]interface{}, name string, tm map[string]interface{}) {
	existing, ok := target[name]
	if !ok { 
		target[name] = tm
		return 
	}
	
	for k, v := range tm {
		if k == "bindings" {
			if newBs, ok := v.([]interface{}); ok {
				for _, nb := range newBs {
					if nbm, ok := nb.(map[string]interface{}); ok {
						p, _ := nbm["protocol"].(string)
						// 支持扁平结构
						attr := make(map[string]interface{})
						if nestedAttr, ok := nbm["attributes"].(map[string]interface{}); ok {
							attr = nestedAttr
						} else {
							for nk, nv := range nbm {
								if nk != "protocol" && nk != "tag" { attr[nk] = nv }
							}
						}
						injectBinding(target, name, p, attr)
					}
				}
			}
		} else { 
			existing[k] = v 
		}
	}
}

func injectBinding(rawTags map[string]map[string]interface{}, tagName, proto string, attr map[string]interface{}) {
	tag, ok := rawTags[tagName]; if !ok { return }
	
	bindings, _ := tag["bindings"].([]interface{})

	// 协议名强制归一化 (大写)
	upProto := strings.ToUpper(proto)

	// 语义归一化：将 scale 统一为 scaler, address 统一为 addr
	normalizedAttr := make(map[string]interface{})
	for k, v := range attr {
		newK := k
		switch strings.ToLower(k) {
		case "scale": newK = "scaler"
		case "address": newK = "addr"
		}
		normalizedAttr[newK] = v
	}

	newB := map[string]interface{}{"protocol": upProto, "attributes": normalizedAttr}
	found := false
	for i, b := range bindings {
		if bm, ok := b.(map[string]interface{}); ok {
			p, _ := bm["protocol"].(string)
			if strings.EqualFold(p, upProto) {
				bindings[i] = newB; found = true; break
			}
		}
	}
	if !found { bindings = append(bindings, newB) }
	tag["bindings"] = bindings
}

func validateProtocolOverlap(active []model.TagDef) {
	mbAddrs := make(map[uint16]string)
	for _, t := range active {
		for _, b := range t.Bindings {
			if strings.EqualFold(b.ProtocolType, "MODBUS") {
				if v, ok := b.Attributes["addr"]; ok {
					var addr uint16
					switch val := v.(type) {
					case int: addr = uint16(val)
					case float64: addr = uint16(val)
					}
					if existing, conflict := mbAddrs[addr]; conflict {
						panic(fmt.Sprintf("FATAL: Modbus Address Conflict at %d! Tags: '%s' and '%s'", addr, existing, t.Name))
					}
					mbAddrs[addr] = t.Name
				}
			}
		}
	}
}
