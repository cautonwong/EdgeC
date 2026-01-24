package core

import (
	"contract-compiler/model"
	"os"
	"path/filepath"
	"sort"
	"strings"

	"gopkg.in/yaml.v3"
)

func processIncludes(includePaths []string, baseDir string, rawTags map[string]map[string]interface{}, effectiveScenarios *[]string, enabledSet map[string]bool, enabledAttr map[string]map[string]interface{}, seen map[string]bool) {
	for _, inc := range includePaths {
		absPath := inc
		if !filepath.IsAbs(inc) && baseDir != "" {
			absPath = filepath.Join(baseDir, inc)
		}
		if seen[absPath] {
			continue
		}
		seen[absPath] = true
		bData, err := os.ReadFile(absPath)
		if err != nil {
			continue
		}
		var raw map[string]interface{}
		yaml.Unmarshal(bData, &raw)
		var subIncludes []string
		if si, ok := raw["include"].([]interface{}); ok {
			for _, item := range si {
				if s, ok := item.(string); ok {
					subIncludes = append(subIncludes, s)
				}
			}
		}
		if si, ok := raw["includes"].([]interface{}); ok {
			for _, item := range si {
				if s, ok := item.(string); ok {
					subIncludes = append(subIncludes, s)
				}
			}
		}
		if len(subIncludes) > 0 {
			processIncludes(subIncludes, filepath.Dir(absPath), rawTags, effectiveScenarios, enabledSet, enabledAttr, seen)
		}
		if en, ok := raw["enabled_tags"].([]interface{}); ok {
			for _, item := range en {
				if s, ok := item.(string); ok {
					enabledSet[s] = true
				} else if m, ok := item.(map[string]interface{}); ok {
					if tagVal, ok := m["tag"].(string); ok {
						enabledSet[tagVal] = true
						if attr, ok := m["attributes"].(map[string]interface{}); ok {
							if enabledAttr[tagVal] == nil {
								enabledAttr[tagVal] = make(map[string]interface{})
							}
							for k, v := range attr {
								enabledAttr[tagVal][k] = v
							}
						}
					}
				}
			}
		}
		proto, _ := raw["protocol"].(string)
		mList, _ := raw["mappings"].([]interface{})
		if len(mList) == 0 {
			mList, _ = raw["map"].([]interface{})
		}
		for _, m := range mList {
			if mm, ok := m.(map[string]interface{}); ok {
				if tagName, ok := mm["tag"].(string); ok {
					injectBinding(rawTags, tagName, proto, mm)
				}
			}
		}
	}
}

func BuildContract(schemaPath, productPath, localPath, baseDir string) *model.EffectiveContract {
	rawTags := make(map[string]map[string]interface{})
	enabledSet := make(map[string]bool)
	loadYamlToRawMap(schemaPath, "tags", rawTags)
	var pc model.ProductConfig
	pData, _ := os.ReadFile(productPath)
	yaml.Unmarshal(pData, &pc)
	effectiveScenarios := []string{}
	seenFiles := make(map[string]bool)
	enabledAttr := make(map[string]map[string]interface{})
	allIncludes := append(pc.Includes, pc.Include...)
	if localPath != "" {
		lData, _ := os.ReadFile(localPath)
		var localMap map[string]interface{}
		yaml.Unmarshal(lData, &localMap)
		if inc, ok := localMap["include"].([]interface{}); ok {
			for _, i := range inc {
				if s, ok := i.(string); ok {
					allIncludes = append(allIncludes, s)
				}
			}
		}
	}
	processIncludes(allIncludes, baseDir, rawTags, &effectiveScenarios, enabledSet, enabledAttr, seenFiles)
	for _, item := range pc.EnabledTags {
		if s, ok := item.(string); ok {
			enabledSet[s] = true
		} else if m, ok := item.(map[string]interface{}); ok {
			if tagName, ok := m["tag"].(string); ok {
				enabledSet[tagName] = true
				if attr, ok := m["attributes"].(map[string]interface{}); ok {
					if enabledAttr[tagName] == nil {
						enabledAttr[tagName] = make(map[string]interface{})
					}
					for k, v := range attr {
						enabledAttr[tagName][k] = v
					}
				}
			}
		}
	}
	effectiveHardware := pc.Hardware
	effectivePipelines := pc.Pipelines
	effectiveParams := pc.Params
	effectiveConfigs := pc.Configs
	effectivePolicies := pc.Policies
	effectiveUI := pc.UI
	effectiveConnectivity := pc.Connectivity
	effectiveDevices := pc.Devices

	if localPath != "" {
		lData, _ := os.ReadFile(localPath)
		var manifest map[string]interface{}
		yaml.Unmarshal(lData, &manifest)
		var mStruct model.ProductConfig
		yaml.Unmarshal(lData, &mStruct)

		// 合并 Connectivity (路)
		if mStruct.Connectivity != nil {
			effectiveConnectivity = mStruct.Connectivity
		}

		// 合并 Devices (车)
		if mStruct.Devices != nil {
			effectiveDevices = mStruct.Devices
		}

		if en, ok := manifest["enabled_tags"].([]interface{}); ok {
			for _, item := range en {
				if s, ok := item.(string); ok {
					enabledSet[s] = true
				} else if m, ok := item.(map[string]interface{}); ok {
					if tagName, ok := m["tag"].(string); ok {
						enabledSet[tagName] = true
						if attr, ok := m["attributes"].(map[string]interface{}); ok {
							if enabledAttr[tagName] == nil {
								enabledAttr[tagName] = make(map[string]interface{})
							}
							for k, v := range attr {
								enabledAttr[tagName][k] = v
							}
						}
					}
				}
			}
		}
		proto := mStruct.Protocol
		if proto == "" {
			proto = "MODBUSRTU"
		}
		for _, m := range mStruct.Mappings {
			if mm, ok := m.(map[string]interface{}); ok {
				if tagName, ok := mm["tag"].(string); ok {
					injectBinding(rawTags, tagName, proto, mm)
				}
			}
		}
		if mStruct.Hardware != nil {
			for _, h := range mStruct.Hardware {
				found := false
				for i := range effectiveHardware {
					eh := &effectiveHardware[i]
					if eh.ID == h.ID || (h.Label != "" && eh.Label == h.Label) {
						if h.Profiles != nil {
							if eh.Profiles == nil {
								eh.Profiles = make(map[string]model.HardwareProfile)
							}
							for k, v := range h.Profiles {
								eh.Profiles[k] = v
							}
						}
						// [核心修复] 按 dev_id 手动合并拓扑结构，防止丢失模拟字段
						if h.Topology != nil {
							for _, newDev := range h.Topology {
								devFound := false
								for j := range eh.Topology {
									if eh.Topology[j].DeviceID == newDev.DeviceID {
										if newDev.Simulation != nil {
											eh.Topology[j].Simulation = newDev.Simulation
										}
										if newDev.Address != 0 {
											eh.Topology[j].Address = newDev.Address
										}
										devFound = true
										break
									}
								}
								if !devFound {
									eh.Topology = append(eh.Topology, newDev)
								}
							}
						}
						if h.Label != "" {
							eh.Label = h.Label
						}
						found = true
						break
					}
				}
				if !found {
					effectiveHardware = append(effectiveHardware, h)
				}
			}
		}
		if mStruct.Pipelines != nil {
			effectivePipelines = mStruct.Pipelines
		}
		if len(mStruct.Params) > 0 {
			effectiveParams = mStruct.Params
		}
		if mStruct.Policies != nil {
			effectivePolicies = mStruct.Policies
		}
		if mStruct.UI.Theme != "" {
			effectiveUI = mStruct.UI
		}
		if mStruct.ProductName != "" {
			pc.ProductName = mStruct.ProductName
		}
	}
	paramBaseID := uint32(10000)
	for i, p := range effectiveParams {
		tagID := paramBaseID + uint32(i)
		effectiveParams[i].ID = tagID
		if _, exists := rawTags[p.Name]; !exists {
			rawTags[p.Name] = map[string]interface{}{
				"name": p.Name, "id": tagID, "type": p.Type, "unit": p.Unit,
				"attributes": map[string]interface{}{"category": "Parameter", "access": "RW", "persist": true},
			}
		}
		enabledSet[p.Name] = true
	}
	var activeTags []model.TagDef
	var allTags []model.TagDef
	for name, rm := range rawTags {
		var t model.TagDef
		b, _ := yaml.Marshal(rm)
		yaml.Unmarshal(b, &t)
		if attr, exists := enabledAttr[name]; exists {
			if t.Attributes == nil {
				t.Attributes = make(map[string]interface{})
			}
			for k, v := range attr {
				t.Attributes[k] = v
			}
		}
		allTags = append(allTags, t)
		if enabledSet[name] {
			activeTags = append(activeTags, t)
		}
	}
	sort.Slice(activeTags, func(i, j int) bool { return activeTags[i].ID < activeTags[j].ID })
	return &model.EffectiveContract{
		Product: pc.ProductName, ActiveTags: activeTags, AllTags: allTags,
		Configs: effectiveConfigs, Params: effectiveParams, Hardware: effectiveHardware,
		Connectivity: effectiveConnectivity, Devices: effectiveDevices,
		Pipelines: effectivePipelines, Policies: effectivePolicies, UI: effectiveUI, Scenarios: effectiveScenarios,
	}
}

func loadYamlToRawMap(path, key string, target map[string]map[string]interface{}) {
	data, err := os.ReadFile(path)
	if err != nil {
		return
	}
	var raw map[string]interface{}
	yaml.Unmarshal(data, &raw)
	if list, ok := raw[key].([]interface{}); ok {
		for _, item := range list {
			if m, ok := item.(map[string]interface{}); ok {
				if name, ok := m["name"].(string); ok {
					target[name] = m
				}
			}
		}
	}
}

func injectBinding(rawTags map[string]map[string]interface{}, tagName, proto string, attr map[string]interface{}) {
	tag, ok := rawTags[tagName]
	if !ok {
		return
	}
	bindings, _ := tag["bindings"].([]interface{})
	upProto := strings.ToUpper(proto)
	if upProto == "" {
		upProto = "MODBUSRTU"
	}
	newB := make(map[string]interface{})
	newB["protocol"] = upProto
	newB["tag"] = tagName
	normalizedAttr := make(map[string]interface{})
	for k, v := range attr {
		if k == "tag" {
			continue
		}
		newK := k
		switch strings.ToLower(k) {
		case "scale":
			newK = "scaler"
		case "address":
			newK = "addr"
		}
		normalizedAttr[newK] = v
	}
	newB["attributes"] = normalizedAttr
	found := false
	for i, b := range bindings {
		if bm, ok := b.(map[string]interface{}); ok {
			p, _ := bm["protocol"].(string)
			if strings.EqualFold(p, upProto) {
				bindings[i] = newB
				found = true
				break
			}
		}
	}
	if !found {
		bindings = append(bindings, newB)
	}
	tag["bindings"] = bindings
}
