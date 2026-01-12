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

// processIncludes 递归处理 YAML 包含逻辑
// 它会合并协议绑定，发现场景文件，并处理嵌套的 includes
func processIncludes(includePaths []string, baseDir string, rawTags map[string]map[string]interface{}, effectiveScenarios *[]string, enabledSet map[string]bool, enabledAttr map[string]map[string]interface{}, seen map[string]bool) {
	for _, inc := range includePaths {
		absPath := inc
		if !filepath.IsAbs(inc) && baseDir != "" {
			absPath = filepath.Join(baseDir, inc)
		}
		
		// 避免循环包含
		if seen[absPath] { continue }
		seen[absPath] = true

		fmt.Printf("[Merger] Processing include: %s\n", absPath)

		// 1. 场景自动发现: 同名的 _scenarios.yaml
		scenFile := strings.TrimSuffix(absPath, filepath.Ext(absPath)) + "_scenarios.yaml"
		if _, err := os.Stat(scenFile); err == nil {
			*effectiveScenarios = append(*effectiveScenarios, scenFile)
		}

		// 2. 读取并解析被包含的文件
		bData, err := os.ReadFile(absPath)
		if err != nil {
			fmt.Printf("[Merger] WARN: Failed to read include: %s\n", absPath)
			continue
		}

		var raw map[string]interface{}
		yaml.Unmarshal(bData, &raw)

		// [核心增强] 模块自带的点位自动启用
		if en, ok := raw["enabled_tags"].([]interface{}); ok {
			for _, item := range en {
				if s, ok := item.(string); ok {
					enabledSet[s] = true
				} else if m, ok := item.(map[string]interface{}); ok {
					if tagVal, exists := m["tag"]; exists {
						if s, ok := tagVal.(string); ok { 
							enabledSet[s] = true 
							if attr, ok := m["attributes"].(map[string]interface{}); ok {
								enabledAttr[s] = attr
							}
						}
					}
				}
			}
		}

		// 3. 处理嵌套包含 (递归)
		if nested, ok := raw["includes"].([]interface{}); ok {
			var nestedStrings []string
			for _, n := range nested {
				if ns, ok := n.(string); ok { nestedStrings = append(nestedStrings, ns) }
			}
			// 递归调用，使用当前文件的目录作为新的 baseDir
			processIncludes(nestedStrings, filepath.Dir(absPath), rawTags, effectiveScenarios, enabledSet, enabledAttr, seen)
		}

		// 4. 合并协议映射
		proto, _ := raw["protocol"].(string)
		mList, _ := raw["mappings"].([]interface{})
		if len(mList) == 0 { mList, _ = raw["map"].([]interface{}) }
		for _, m := range mList {
			if mm, ok := m.(map[string]interface{}); ok {
				if tagName, ok := mm["tag"].(string); ok { injectBinding(rawTags, tagName, proto, mm) }
			}
		}
		
		// 5. [演习增强] 合并场景列表 (显式定义的场景)
		if scenRaw, ok := raw["scenarios"].([]interface{}); ok {
			for _, s := range scenRaw {
				if ss, ok := s.(string); ok {
					sAbs := ss
					if !filepath.IsAbs(ss) { sAbs = filepath.Join(filepath.Dir(absPath), ss) }
					*effectiveScenarios = append(*effectiveScenarios, sAbs)
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
	
	var effectiveScenarios []string
	seenFiles := make(map[string]bool)
	enabledAttr := make(map[string]map[string]interface{})

	// 1. 处理 Base Product 的包含
	processIncludes(pc.Includes, baseDir, rawTags, &effectiveScenarios, enabledSet, enabledAttr, seenFiles)

	for _, item := range pc.EnabledTags {
		if s, ok := item.(string); ok {
			enabledSet[s] = true
		} else if m, ok := item.(map[string]interface{}); ok {
			if tagVal, exists := m["tag"]; exists {
				if s, ok := tagVal.(string); ok { 
					enabledSet[s] = true 
					if attr, ok := m["attributes"].(map[string]interface{}); ok {
						enabledAttr[s] = attr
					}
				}
			}
		} else if m, ok := item.(map[interface{}]interface{}); ok {
			for k, v := range m {
				if ks, ok := k.(string); ok && ks == "tag" {
					if vs, ok := v.(string); ok { 
						enabledSet[vs] = true 
						if attr, ok := v.(map[string]interface{}); ok {
							enabledAttr[vs] = attr
						}
					}
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

	// 2. 处理 Local Extension
	if localPath != "" {
		if lData, err := os.ReadFile(localPath); err == nil {
			var local map[string]interface{}
			yaml.Unmarshal(lData, &local)
			fmt.Printf("[Merger] Loading local extension: %s\n", localPath)

			// [关键点] 同样支持 extension.yaml 里的 includes 递归解析
			if inc, ok := local["includes"].([]interface{}); ok {
				var incStrs []string
				for _, i := range inc {
					if s, ok := i.(string); ok { incStrs = append(incStrs, s) }
				}
				processIncludes(incStrs, filepath.Dir(localPath), rawTags, &effectiveScenarios, enabledSet, enabledAttr, seenFiles)
			}

			if en, ok := local["enabled_tags"].([]interface{}); ok {
				for _, n := range en {
					if s, ok := n.(string); ok {
						enabledSet[s] = true
					} else if m, ok := n.(map[string]interface{}); ok {
						if tagName, ok := m["tag"].(string); ok { 
							enabledSet[tagName] = true 
							if attr, ok := m["attributes"].(map[string]interface{}); ok {
								enabledAttr[tagName] = attr
							}
						}
					}
				}
			}

			// [Mask Rule] 处理屏蔽逻辑
			if dis, ok := local["disabled_tags"].([]interface{}); ok {
				for _, n := range dis {
					if s, ok := n.(string); ok {
						fmt.Printf("[Merger] Masking Tag: %s\n", s)
						delete(enabledSet, s)
					}
				}
			}
			if cfgRaw, ok := local["configs"].([]interface{}); ok {
				var localConfigs []model.ConfigDef
				for _, c := range cfgRaw {
					if cm, ok := c.(map[string]interface{}); ok {
						name, _ := cm["name"].(string)
						if name != "" { localConfigs = append(localConfigs, model.ConfigDef{Name: name, Value: cm["value"]})
						}
					}
				}
				mergeConfigs(&effectiveConfigs, localConfigs)
			}
						            // [Override Rule] Merge Parameters
						            if paramRaw, ok := local["parameters"].([]interface{}); ok {
						                for _, p := range paramRaw {
						                    if pm, ok := p.(map[string]interface{}); ok {
						                        name, _ := pm["name"].(string)
						                        found := false
						                        for i, ep := range effectiveParams {
						                            if ep.Name == name {
						                                pb, _ := yaml.Marshal(pm)
						                                yaml.Unmarshal(pb, &effectiveParams[i])
			                                            // [关键修复] 显式回填覆盖值到默认值字段
			                                            if val, exists := pm["value"]; exists { effectiveParams[i].Default = val }
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
			// [Override Rule] Merge Policies
			if polRaw, ok := local["policies"].([]interface{}); ok {
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
				pb, _ := yaml.Marshal(uiRaw)
				yaml.Unmarshal(pb, &effectiveUI)
			}

			// Local Scenario Scanning
			localDir := filepath.Dir(localPath)
			scenDir := filepath.Join(localDir, "scenarios")
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
		
		// [核心修复] 合并 YAML 中定义的属性
		if attr, exists := enabledAttr[name]; exists {
			if t.Attributes == nil { t.Attributes = make(map[string]interface{}) }
			for k, v := range attr { t.Attributes[k] = v }
		}

		allTags = append(allTags, t)
		if enabledSet[name] { 
			activeTags = append(activeTags, t) 
		}
	}

	paramBaseID := uint32(10000)
	for i, p := range effectiveParams {
		tagID := paramBaseID + uint32(i)
		effectiveParams[i].ID = tagID
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
	
	newB := make(map[string]interface{})
	newB["protocol"] = upProto
	newB["tag"] = tagName
	
	normalizedAttr := make(map[string]interface{})
	for k, v := range attr {
		if k == "tag" { continue }
		newK := k
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