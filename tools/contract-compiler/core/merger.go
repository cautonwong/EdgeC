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
	rawTags := make(map[string]map[string]interface{})
	enabledSet := make(map[string]bool)
	loadYamlToRawMap(schemaPath, "tags", rawTags)

	var pc model.ProductConfig
	pData, _ := os.ReadFile(productPath)
	yaml.Unmarshal(pData, &pc)
	
	var effectiveScenarios []string
	for _, inc := range pc.Includes {
		absPath := inc
		if !filepath.IsAbs(inc) && baseDir != "" { absPath = filepath.Join(baseDir, inc) }
		
		// Discovery: same-named _scenarios.yaml
		scenFile := strings.TrimSuffix(absPath, filepath.Ext(absPath)) + "_scenarios.yaml"
		if _, err := os.Stat(scenFile); err == nil {
			effectiveScenarios = append(effectiveScenarios, scenFile)
		}

		if bData, err := os.ReadFile(absPath); err == nil {
			var raw map[string]interface{}
			yaml.Unmarshal(bData, &raw)
			proto, _ := raw["protocol"].(string)
			mList, _ := raw["mappings"].([]interface{})
			if len(mList) == 0 { mList, _ = raw["map"].([]interface{}) }
			for _, m := range mList {
				if mm, ok := m.(map[string]interface{}); ok {
					if tagName, ok := mm["tag"].(string); ok { injectBinding(rawTags, tagName, proto, mm) }
				}
			}
		}
	}

	for _, item := range pc.EnabledTags {
		if s, ok := item.(string); ok {
			enabledSet[s] = true
		} else if m, ok := item.(map[string]interface{}); ok {
			// 尝试从 "tag" 键提取
			if tagVal, exists := m["tag"]; exists {
				if s, ok := tagVal.(string); ok { enabledSet[s] = true }
			}
		} else if m, ok := item.(map[interface{}]interface{}); ok {
			// 处理可能的 interface{} 键 (yaml.v3 边缘情况)
			for k, v := range m {
				if ks, ok := k.(string); ok && ks == "tag" {
					if vs, ok := v.(string); ok { enabledSet[vs] = true }
				}
			}
		}
	}

	effectiveConfigs := pc.Configs
	effectiveParams := pc.Params
	effectiveHardware := pc.Hardware
	effectivePipelines := pc.Pipelines
	effectivePolicies := pc.Policies
	effectiveUI := pc.UI

	if localPath != "" {
		if lData, err := os.ReadFile(localPath); err == nil {
			var local map[string]interface{}
			yaml.Unmarshal(lData, &local)
			fmt.Printf("[Merger] Loading local extension: %s\n", localPath)
			if en, ok := local["enabled_tags"].([]interface{}); ok {
				fmt.Printf("[Merger] Found %d enabled_tags in extension\n", len(en))
				// [关键修复] 增量合并，不要覆盖
				for _, n := range en {
					if s, ok := n.(string); ok {
						enabledSet[s] = true
					} else if m, ok := n.(map[string]interface{}); ok {
						if tagName, ok := m["tag"].(string); ok { enabledSet[tagName] = true }
					}
				}
			}
			            if cfgRaw, ok := local["configs"].([]interface{}); ok {
			                fmt.Printf("[Merger] Found %d configs in extension\n", len(cfgRaw))
			                var localConfigs []model.ConfigDef
			                for _, c := range cfgRaw {
			                    if cm, ok := c.(map[string]interface{}); ok {
			                        name, _ := cm["name"].(string)
			                        if name != "" { localConfigs = append(localConfigs, model.ConfigDef{Name: name, Value: cm["value"]}) }
			                    }
			                }
			                mergeConfigs(&effectiveConfigs, localConfigs)
			            }
			            // [Override Rule] Merge Parameters (Runtime Tunables)
			            if paramRaw, ok := local["parameters"].([]interface{}); ok {
			                fmt.Printf("[Merger] Found %d parameters in extension\n", len(paramRaw))
			                for _, p := range paramRaw {
			                    if pm, ok := p.(map[string]interface{}); ok {
			                        name, _ := pm["name"].(string)
			                        found := false
			                        for i, ep := range effectiveParams {
			                            if ep.Name == name {
			                                pb, _ := yaml.Marshal(pm)
			                                yaml.Unmarshal(pb, &effectiveParams[i])
			                                found = true; break
			                            }
			                        }
			                        if !found {
			                            var pd model.ParamDef
			                            pb, _ := yaml.Marshal(pm)
			                            yaml.Unmarshal(pb, &pd)
			                            if pd.Default == nil && pd.Value != nil { pd.Default = pd.Value }
			                            effectiveParams = append(effectiveParams, pd)
			                        }
			                    }
			                }
			            }
			            // [Override Rule] Merge Hardware
			            if hwRaw, ok := local["hardware"].([]interface{}); ok {
			                fmt.Printf("[Merger] Found %d hardware resources in extension\n", len(hwRaw))
			                for _, h := range hwRaw {
			                    if hm, ok := h.(map[string]interface{}); ok {
			                        label, _ := hm["label"].(string)
			                        found := false
			                        for i, eh := range effectiveHardware {
			                            if eh.Label == label {
			                                pb, _ := yaml.Marshal(hm)
			                                yaml.Unmarshal(pb, &effectiveHardware[i])
			                                found = true; break
			                            }
			                        }
			                        if !found {
			                            var hd model.HardwareDef
			                            pb, _ := yaml.Marshal(hm)
			                            yaml.Unmarshal(pb, &hd)
			                            effectiveHardware = append(effectiveHardware, hd)
			                        }
			                    }
			                }
			            }
			            // [Override Rule] Merge Pipelines
			            if pipeRaw, ok := local["pipelines"].([]interface{}); ok {
			                fmt.Printf("[Merger] Found %d pipelines in extension\n", len(pipeRaw))
			                for _, p := range pipeRaw {
			                    if pm, ok := p.(map[string]interface{}); ok {
			                        id, _ := pm["id"].(string)
			                        found := false
			                        for i, ep := range effectivePipelines {
			                            if ep.ID == id {
			                                pb, _ := yaml.Marshal(pm)
			                                yaml.Unmarshal(pb, &effectivePipelines[i])
			                                found = true; break
			                            }
			                        }
			                        if !found {
			                            var pd model.PipelineDef
			                            pb, _ := yaml.Marshal(pm)
			                            yaml.Unmarshal(pb, &pd)
			                            effectivePipelines = append(effectivePipelines, pd)
			                        }
			                    }
			                }
			            }
			            // [Override Rule] Merge Policies			// [Override Rule] Merge Policies
			if polRaw, ok := local["policies"].([]interface{}); ok {
				fmt.Printf("[Merger] Found %d policies in extension\n", len(polRaw))
				for _, p := range polRaw {
					if pm, ok := p.(map[string]interface{}); ok {
						name, _ := pm["name"].(string)
						found := false
						for i, ep := range effectivePolicies {
							if ep.Name == name {
								pb, _ := yaml.Marshal(pm)
								yaml.Unmarshal(pb, &effectivePolicies[i])
								found = true; break
							}
						}
						if !found {
							var pd model.PolicyDef
							pb, _ := yaml.Marshal(pm)
							yaml.Unmarshal(pb, &pd)
							effectivePolicies = append(effectivePolicies, pd)
						}
					}
				}
			}
			// [Override Rule] Merge UI
			if uiRaw, ok := local["ui"].(map[string]interface{}); ok {
				fmt.Printf("[Merger] Found UI config in extension\n")
				pb, _ := yaml.Marshal(uiRaw)
				yaml.Unmarshal(pb, &effectiveUI)
			}

			// Local Scenario Scanning
			localDir := filepath.Dir(localPath)
			scenDir := filepath.Join(localDir, "scenarios")
			fmt.Printf("[Merger] Scanning scenarios in: %s\n", scenDir)
			if _, err := os.Stat(scenDir); err == nil {
				filepath.Walk(scenDir, func(path string, info os.FileInfo, err error) error {
					if err == nil && !info.IsDir() && (strings.HasSuffix(path, ".yaml") || strings.HasSuffix(path, ".yml")) {
						effectiveScenarios = append(effectiveScenarios, path)
					}
					return nil
				})
			}
		}
	}

	var activeTags []model.TagDef
	var allTags []model.TagDef
	usedIDs := make(map[uint32]string)
	for name, rm := range rawTags {
		var t model.TagDef
		b, _ := yaml.Marshal(rm)
		yaml.Unmarshal(b, &t)
		if t.ID != 0 {
			if existing, conflict := usedIDs[t.ID]; conflict && existing != name { panic(fmt.Sprintf("ID Conflict! %d", t.ID)) }
			usedIDs[t.ID] = name
		}
		allTags = append(allTags, t)
		if enabledSet[name] { 
			fmt.Printf("[Merger] FINAL: Enabling Tag: %s (ID: %d)\n", name, t.ID)
			activeTags = append(activeTags, t) 
		} else {
			// Debug: why is it not enabled?
			// fmt.Printf("[Merger] FINAL: Tag %s is NOT in enabledSet\n", name)
		}
	}

	// [归一化] 将 Parameters 转换为虚拟 Tags 并分配 10000+ ID
	paramBaseID := uint32(10000)
	for i, p := range effectiveParams {
		tagID := paramBaseID + uint32(i)
		effectiveParams[i].ID = tagID // 回填到 Parameter 定义中

		// 创建影子 Tag
		pTag := model.TagDef{
			ID:    tagID,
			Name:  p.Name,
			Type:  p.Type,
			Unit:  p.Unit,
			Desc:  p.Desc,
			Attributes: map[string]interface{}{
				"category": "Parameter",
				"access":   "RW",
				"persist":  true,
				"min":      p.Min,
				"max":      p.Max,
			},
			Bindings: p.Bindings,
		}
		activeTags = append(activeTags, pTag)
		allTags = append(allTags, pTag)
	}

	sort.Slice(activeTags, func(i, j int) bool { return activeTags[i].ID < activeTags[j].ID })
	sort.Slice(allTags, func(i, j int) bool { return allTags[i].ID < allTags[j].ID })
	sort.Slice(effectiveConfigs, func(i, j int) bool { return effectiveConfigs[i].Name < effectiveConfigs[j].Name })
	sort.Slice(effectiveParams, func(i, j int) bool { return effectiveParams[i].Name < effectiveParams[j].Name })

	return &model.EffectiveContract{ 
		Product: pc.ProductName, 
		ActiveTags: activeTags, 
		AllTags: allTags, 
		Configs: effectiveConfigs, 
		Params: effectiveParams,
		Hardware: effectiveHardware,
		Pipelines: effectivePipelines,
		Policies: effectivePolicies,
		UI: effectiveUI,
		Scenarios: effectiveScenarios,
	}
}

func mergeConfigs(target *[]model.ConfigDef, source []model.ConfigDef) {
	for _, s := range source {
		found := false
		for i, t := range *target { if t.Name == s.Name { (*target)[i].Value = s.Value; found = true; break } }
		if !found { *target = append(*target, s) }
	}
}

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

func injectBinding(rawTags map[string]map[string]interface{}, tagName, proto string, attr map[string]interface{}) {
	tag, ok := rawTags[tagName]; if !ok { return }
	bindings, _ := tag["bindings"].([]interface{})
	upProto := strings.ToUpper(proto)
	
	// [关键修复] 严格对齐 model.Binding 结构体
	newB := make(map[string]interface{})
	newB["protocol"] = upProto
	newB["tag"] = tagName
	
	// 协议特定属性必须放在 attributes 嵌套中
	normalizedAttr := make(map[string]interface{})
	for k, v := range attr {
		if k == "tag" { continue }
		newK := k
		// 统一映射 scale -> scaler, address -> addr 等别名 (如果需要)
		switch strings.ToLower(k) {
		case "scale": newK = "scaler"
		case "address": newK = "addr"
		}
		normalizedAttr[newK] = v
	}
	newB["attributes"] = normalizedAttr
	
	found := false
	for i, b := range bindings {
		if bm, ok := b.(map[string]interface{}); ok {
			p, _ := bm["protocol"].(string)
			if strings.EqualFold(p, upProto) { bindings[i] = newB; found = true; break }
		}
	}
	if !found { bindings = append(bindings, newB) }
	tag["bindings"] = bindings
}
